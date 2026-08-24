// A game flow of three levels, one cutscene and one demo. The second level is a
// gym, which is the case worth having: a gym has no number, so the ordinal a
// script reads is not the level's place in the table.

#include <fakes/game.h>

#include <harness/fake_calls.h>

#include <trx/game/demo.h>
#include <trx/game/game/enum.h>
#include <trx/game/game/state.h>
#include <trx/game/game_flow/common.h>
#include <trx/game/savegame.h>
#include <trx/game/screenshot.h>

#include <string.h>

#include <lauxlib.h>

static GF_LEVEL m_Levels[FAKE_LEVEL_COUNT];
static GF_LEVEL m_Cutscenes[FAKE_CUTSCENE_COUNT];
static GF_LEVEL m_Demos[FAKE_DEMO_COUNT];
static GF_LEVEL m_TitleLevel;
static GF_LEVEL_TABLE m_Tables[GFLT_NUMBER_OF];
static const GF_LEVEL *m_CurrentLevel;
static bool m_HasGym;
static bool m_InCutscene;

// The bonus start is a passport choice, so a test says whether this run is one.
static bool m_IsNGPlus;

static void M_Reset(void)
{
    memset(m_Levels, 0, sizeof(m_Levels));
    memset(m_Cutscenes, 0, sizeof(m_Cutscenes));
    memset(m_Demos, 0, sizeof(m_Demos));

    m_Levels[0] = (GF_LEVEL) {
        .num = 0,
        .type = GFL_GYM,
        .title = "Lara's Home",
        .path = "gym.phd",
        .key = "gym",
        .lara_outfit = "casual",
        .water_particles = false,
    };
    m_Levels[1] = (GF_LEVEL) {
        .num = 1,
        .type = GFL_NORMAL,
        .title = "Caves",
        .path = "level1.phd",
        .key = "level1",
        .script_path = "caves.lua",
        .lara_outfit = "default",
        .water_particles = true,
        .unobtainable = { .pickups = 1, .secrets = 2 },
    };
    m_Levels[2] = (GF_LEVEL) {
        .num = 2,
        .type = GFL_NORMAL,
        .title = "Vilcabamba",
        .path = "level2.phd",
        .key = "level2",
    };
    m_Cutscenes[0] = (GF_LEVEL) {
        .num = 0,
        .type = GFL_CUTSCENE,
        .title = "Cutscene 1",
    };
    m_Demos[0] = (GF_LEVEL) {
        .num = 0,
        .type = GFL_DEMO,
        .title = "Demo 1",
    };
    m_TitleLevel = (GF_LEVEL) {
        .num = 0,
        .type = GFL_TITLE,
        .title = "Title",
    };

    m_Tables[GFLT_MAIN] =
        (GF_LEVEL_TABLE) { .count = FAKE_LEVEL_COUNT, .levels = m_Levels };
    m_Tables[GFLT_CUTSCENES] = (GF_LEVEL_TABLE) { .count = FAKE_CUTSCENE_COUNT,
                                                  .levels = m_Cutscenes };
    m_Tables[GFLT_DEMOS] =
        (GF_LEVEL_TABLE) { .count = FAKE_DEMO_COUNT, .levels = m_Demos };
    m_Tables[GFLT_TITLE] = (GF_LEVEL_TABLE) { .count = 0, .levels = nullptr };

    m_CurrentLevel = nullptr;
    m_IsNGPlus = false;
    m_HasGym = true;
    m_InCutscene = false;
}

// fake.set_current_level(n) - nil for a game that is not in a level at all.
static int M_L_SetCurrentLevel(lua_State *const L)
{
    FakeGame_SetCurrentLevel(
        lua_isnil(L, 1) ? -1 : (int32_t)luaL_checkinteger(L, 1) - 1);
    return 0;
}

// fake.set_current_title() - the title level, which is not in any table.
static int M_L_SetCurrentTitle(lua_State *const L)
{
    m_CurrentLevel = &m_TitleLevel;
    return 0;
}

// fake.set_in_cutscene(bool) - a level is loaded, but the game takes no input.
static int M_L_SetInCutscene(lua_State *const L)
{
    FakeGame_SetInCutscene(lua_toboolean(L, 1));
    return 0;
}

// fake.set_gym_present(bool) - whether the flow has a gym to play.
static int M_L_SetGymPresent(lua_State *const L)
{
    FakeGame_SetGymPresent(lua_toboolean(L, 1));
    return 0;
}

// fake.set_ngplus(bool) - whether this run started from the bonus entry.
static int M_L_SetNGPlus(lua_State *const L)
{
    FakeGame_SetNGPlus(lua_toboolean(L, 1));
    return 0;
}

void Screenshot_Make(const SCREENSHOT_FORMAT format)
{
}

void Screenshot_MakeToPath(const char *const path)
{
}

void Game_SetIsLevelComplete(const bool is_complete)
{
    FAKE_RECORD("end_level");
}

const GF_LEVEL_TABLE *GF_GetLevelTable(const GF_LEVEL_TABLE_TYPE table_type)
{
    if (table_type < 0 || table_type >= GFLT_NUMBER_OF) {
        return nullptr;
    }
    return &m_Tables[table_type];
}

// As the real one: a gym is in the table but is not one of the levels the game
// numbers, so it is not counted either.
int32_t GF_GetLevelCount(const GF_LEVEL_TABLE_TYPE table_type)
{
    const GF_LEVEL_TABLE *const tbl = GF_GetLevelTable(table_type);
    if (tbl == nullptr) {
        return 0;
    }
    int32_t count = 0;
    for (int32_t i = 0; i < tbl->count; i++) {
        if (tbl->levels[i].type != GFL_GYM) {
            count++;
        }
    }
    return count;
}

const GF_LEVEL *GF_GetLevel(
    const GF_LEVEL_TABLE_TYPE table_type, const int32_t num)
{
    const GF_LEVEL_TABLE *const tbl = GF_GetLevelTable(table_type);
    if (tbl == nullptr || num < 0 || num >= tbl->count) {
        return nullptr;
    }
    return &tbl->levels[num];
}

const GF_LEVEL *GF_GetTitleLevel(void)
{
    return &m_TitleLevel;
}

const GF_LEVEL *GF_GetGymLevel(void)
{
    if (!m_HasGym) {
        return nullptr;
    }
    const GF_LEVEL_TABLE *const tbl = GF_GetLevelTable(GFLT_MAIN);
    for (int32_t i = 0; i < tbl->count; i++) {
        if (tbl->levels[i].type == GFL_GYM) {
            return &tbl->levels[i];
        }
    }
    return nullptr;
}

const GF_LEVEL *GF_GetCurrentLevel(void)
{
    return m_CurrentLevel;
}

// The game flow's level and the game's are not the same one. The title screen
// is a level the flow is on and the game is not, which is where a caller that
// reads the game's and guards on the flow's comes apart.
const GF_LEVEL *Game_GetCurrentLevel(void)
{
    return m_CurrentLevel == &m_TitleLevel ? nullptr : m_CurrentLevel;
}

GF_LEVEL_TABLE_TYPE GF_GetLevelTableType(const GF_LEVEL_TYPE level_type)
{
    switch (level_type) {
    case GFL_TITLE:
        return GFLT_TITLE;
    case GFL_CUTSCENE:
        return GFLT_CUTSCENES;
    case GFL_DEMO:
        return GFLT_DEMOS;
    default:
        return GFLT_MAIN;
    }
}

int32_t GF_GetLevelOrdinalNumber(
    const GF_LEVEL_TABLE_TYPE table_type, const GF_LEVEL *const ref_level)
{
    const GF_LEVEL_TABLE *const tbl = GF_GetLevelTable(table_type);
    int32_t ordinal = 1;
    for (int32_t i = 0; i < tbl->count; i++) {
        const GF_LEVEL *const level = &tbl->levels[i];
        if (level == ref_level) {
            return level->type == GFL_GYM ? 0 : ordinal;
        }
        if (level->type != GFL_GYM) {
            ordinal++;
        }
    }
    return 0;
}

// The next demo to play: with no hint, the first; otherwise the one asked for.
int32_t Demo_ChooseLevel(const int32_t demo_num)
{
    if (FAKE_DEMO_COUNT == 0) {
        return -1;
    }
    return demo_num < 0 ? 0 : demo_num;
}

GF_LEVEL *GF_GetLevelByOrdinalNumber(
    const GF_LEVEL_TABLE_TYPE table_type, const int32_t level_num)
{
    const GF_LEVEL_TABLE *const tbl = GF_GetLevelTable(table_type);
    if (tbl == nullptr) {
        return nullptr;
    }
    for (int32_t i = 0; i < tbl->count; i++) {
        GF_LEVEL *const level = &tbl->levels[i];
        if (GF_GetLevelOrdinalNumber(table_type, level) == level_num) {
            return level;
        }
    }
    return nullptr;
}

// The gameflow command the play_* verbs queue, and the savegame bookkeeping
// they do on the way. Recorded, not performed.
void GF_OverrideCommand(const GF_COMMAND command)
{
    const int32_t num = command.param;
    switch (command.action) {
    case GF_START_GAME:
        FAKE_RECORD("play_level", FV(num));
        break;
    case GF_START_CINE:
        FAKE_RECORD("play_cutscene", FV(num));
        break;
    case GF_START_DEMO:
        FAKE_RECORD("play_demo", FV(num));
        break;
    case GF_SELECT_GAME:
        FAKE_RECORD("play_gym", FV(num));
        break;
    default:
        break;
    }
}

void SG_Resume_StoreGameToEntry(const GF_LEVEL *const level)
{
}

RESUME_INFO *SG_Resume_GetEntry(const GF_LEVEL *const level)
{
    return nullptr;
}

int32_t g_TRVersion = 1;
const char *g_TRXVersion = "TRX-test";

bool Game_IsLoaded(void)
{
    return m_CurrentLevel != nullptr;
}

bool Game_IsBonusFlagSet(const GAME_BONUS_FLAG flag)
{
    return flag == GBF_NGPLUS && m_IsNGPlus;
}

void FakeGame_SetNGPlus(const bool ngplus)
{
    m_IsNGPlus = ngplus;
}

bool Game_IsPlayable(void)
{
    return m_CurrentLevel != nullptr && !m_InCutscene;
}

bool Game_IsPlaying(void)
{
    return true;
}

double Clock_GetRealTime(void)
{
    return 0.0;
}

bool PhotoMode_IsActive(void)
{
    return false;
}

void FakeGame_SetInCutscene(const bool in_cutscene)
{
    m_InCutscene = in_cutscene;
}

FAKE_ON_RESET(M_Reset)

void FakeGame_SetGymPresent(const bool present)
{
    m_HasGym = present;
}

void FakeGame_SetCurrentLevel(const int32_t idx)
{
    m_CurrentLevel = idx < 0 ? nullptr : &m_Levels[idx];
}

void FakeGame_PushLua(lua_State *const L)
{
    lua_pushcfunction(L, M_L_SetNGPlus);
    lua_setfield(L, -2, "set_ngplus");
    lua_pushcfunction(L, M_L_SetCurrentLevel);
    lua_setfield(L, -2, "set_current_level");
    lua_pushcfunction(L, M_L_SetCurrentTitle);
    lua_setfield(L, -2, "set_current_title");
    lua_pushcfunction(L, M_L_SetInCutscene);
    lua_setfield(L, -2, "set_in_cutscene");
    lua_pushcfunction(L, M_L_SetGymPresent);
    lua_setfield(L, -2, "set_gym_present");
    lua_pushinteger(L, FAKE_LEVEL_COUNT);
    lua_setfield(L, -2, "LEVEL_COUNT");
    // The levels the game numbers: the gym is in the table but is not one.
    lua_pushinteger(L, FAKE_LEVEL_COUNT - 1);
    lua_setfield(L, -2, "NUMBERED_LEVEL_COUNT");
}

int32_t Output_GetMeasuredFPS(void)
{
    return 0;
}
