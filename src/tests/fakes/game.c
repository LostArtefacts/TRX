// A game flow of three levels, one cutscene and one demo. The second level is a
// gym, which is the case worth having: a gym has no number, so the ordinal a
// script reads is not the level's place in the table.

#include <fakes/game.h>

#include <harness/fake_calls.h>

#include <trx/game/demo.h>
#include <trx/game/game/enum.h>
#include <trx/game/game/state.h>
#include <trx/game/game_flow/common.h>
#include <trx/game/inventory_ring/control.h>
#include <trx/game/inventory_ring/types.h>
#include <trx/game/photo_mode.h>
#include <trx/game/savegame.h>
#include <trx/game/screenshot.h>

#include <string.h>

#include <lauxlib.h>

static GF_LEVEL m_Levels[FAKE_LEVEL_COUNT];
static GF_LEVEL m_Cutscenes[FAKE_CUTSCENE_COUNT];
static GF_LEVEL m_Demos[FAKE_DEMO_COUNT];
static GF_FMV m_FMVs[FAKE_FMV_COUNT];
static GF_LEVEL m_TitleLevel;
static GF_LEVEL_TABLE m_Tables[GFLT_NUMBER_OF];
static RESUME_INFO m_Resume;
static const GF_LEVEL *m_CurrentLevel;
static bool m_HasGym;
static bool m_InCutscene;

// The bonus start is a passport choice, so a test says whether this run is one.
static bool m_IsNGPlus;
static bool m_IsPlaying = true;

static bool m_InPhotoMode;
static PHOTO_MODE m_PhotoModeTarget;

// The open ring, holding the one entry that the player has picked out of it.
static bool m_RingOpen;
static INV_RING m_Ring;
static INVENTORY_ITEM m_RingItem;
static INVENTORY_ITEM *m_RingList[1] = { &m_RingItem };

// Whether a script holds an engine screen, which makes the play_* verbs wait.
static bool m_ScreenHeld;

static void M_Reset(void)
{
    m_RingOpen = false;
    m_ScreenHeld = false;
    m_InPhotoMode = false;
    m_PhotoModeTarget = PHOTO_MODE_CAMERA;
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
    m_FMVs[0] = (GF_FMV) { .path = "fmv/legal.rpl", .is_legal = true };
    m_FMVs[1] = (GF_FMV) { .path = "fmv/intro.rpl", .is_intro = true };
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
    m_IsPlaying = true;
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

// fake.set_current_cutscene() - the first cutscene level.
static int M_L_SetCurrentCutscene(lua_State *const L)
{
    m_CurrentLevel = &m_Cutscenes[0];
    return 0;
}

// fake.set_in_cutscene(bool) - a level is loaded, but the game takes no input.
static int M_L_SetInCutscene(lua_State *const L)
{
    FakeGame_SetInCutscene(lua_toboolean(L, 1));
    return 0;
}

// fake.set_photo_mode(bool, [bool]) - whether photo mode is open, and whether
// it is steering Lara rather than the camera.
static int M_L_SetPhotoMode(lua_State *const L)
{
    FakeGame_SetPhotoMode(
        lua_toboolean(L, 1),
        lua_toboolean(L, 2) ? PHOTO_MODE_LARA_POS : PHOTO_MODE_CAMERA);
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

// fake.open_ring(mode, object_id, open_frame, frame_count) - a ring opened for
// the mode, resting on an entry that the player has picked.
static int M_L_OpenRing(lua_State *const L)
{
    m_Ring = (INV_RING) {
        .mode = (INVENTORY_MODE)luaL_checkinteger(L, 1),
        .list = m_RingList,
        .number_of_objects = 1,
        .current_object = 0,
    };
    m_RingItem = (INVENTORY_ITEM) {
        .object_id = (OBJECT_ID)luaL_checkinteger(L, 2),
        .open_frame = (int16_t)luaL_checkinteger(L, 3),
        .frames_total = (int16_t)luaL_checkinteger(L, 4),
    };
    m_RingItem.current_frame = m_RingItem.open_frame;
    m_RingItem.goal_frame = m_RingItem.open_frame;
    m_RingOpen = true;
    return 0;
}

// fake.close_ring()
static int M_L_CloseRing(lua_State *const L)
{
    m_RingOpen = false;
    return 0;
}

// fake.settle_ring() -> frame - the picked entry runs to the frame that a
// script asked for, as the ring animates it over the next few ticks.
static int M_L_SettleRing(lua_State *const L)
{
    m_RingItem.current_frame = m_RingItem.goal_frame;
    lua_pushinteger(L, m_RingItem.current_frame);
    return 1;
}

// fake.set_screen_held(bool) - whether a script holds an engine screen.
static int M_L_SetScreenHeld(lua_State *const L)
{
    m_ScreenHeld = lua_toboolean(L, 1);
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

int32_t GF_GetFMVCount(void)
{
    return FAKE_FMV_COUNT;
}

const GF_FMV *GF_GetFMV(const int32_t num)
{
    if (num < 1 || num > FAKE_FMV_COUNT) {
        return nullptr;
    }
    return &m_FMVs[num - 1];
}

int32_t GF_GetFMVNumber(const GF_FMV *const fmv)
{
    return (int32_t)(fmv - m_FMVs) + 1;
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

const GF_LEVEL *GF_GetFirstLevel(void)
{
    const GF_LEVEL_TABLE *const tbl = GF_GetLevelTable(GFLT_MAIN);
    for (int32_t i = 0; i < tbl->count; i++) {
        if (tbl->levels[i].type != GFL_GYM) {
            return &tbl->levels[i];
        }
    }
    return nullptr;
}

void Game_SetBonusFlag(const GAME_BONUS_FLAG flag)
{
    m_IsNGPlus = flag == GBF_NGPLUS;
}

void SG_Resume_ResetAllEntries(void)
{
    FAKE_RECORD("reset_resume");
}

void SG_Manager_UnbindSlot(void)
{
    FAKE_RECORD("unbind_slot");
}

void SG_Manager_BindSlot(const SAVEGAME_SLOT_REF slot)
{
    const int32_t index = slot.index;
    FAKE_RECORD("bind_slot", FV(index));
}

SAVEGAME_SLOT_REF SG_Manager_NormalSlot(const int32_t index)
{
    return (SAVEGAME_SLOT_REF) { .pool = SAVEGAME_SLOT_POOL_NORMAL,
                                 .index = index };
}

SAVEGAME_SLOT_REF SG_Manager_QuickFromVisualIndex(const int32_t visual_index)
{
    return (SAVEGAME_SLOT_REF) { .pool = SAVEGAME_SLOT_POOL_QUICK,
                                 .index = visual_index };
}

SAVEGAME_SLOT_REF SG_Manager_InvalidSlot(void)
{
    return (SAVEGAME_SLOT_REF) { .index = -1 };
}

bool SG_Manager_IsValidSlotRef(const SAVEGAME_SLOT_REF slot)
{
    return slot.index >= 0;
}

INV_RING *InvRing_GetActiveRing(void)
{
    return m_RingOpen ? &m_Ring : nullptr;
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
void GF_OverrideCommand(const GF_COMMAND command, const bool immediate)
{
    const int32_t num = command.param;
    switch (command.action) {
    case GF_START_GAME:
        FAKE_RECORD("play_level", FV(num), FV(immediate));
        break;
    case GF_START_CINE:
        FAKE_RECORD("play_cutscene", FV(num));
        break;
    case GF_START_DEMO:
        FAKE_RECORD("play_demo", FV(num));
        break;
    case GF_START_FMV:
        FAKE_RECORD("play_fmv", FV(num));
        break;
    case GF_SELECT_GAME:
        FAKE_RECORD("play_gym", FV(num), FV(immediate));
        break;
    case GF_RESTART_GAME:
        FAKE_RECORD("restart_level", FV(num));
        break;
    case GF_NEW_GAME: {
        const bool ng_plus = command.param == GBF_NGPLUS;
        FAKE_RECORD("new_game", FV(ng_plus), FV(immediate));
        break;
    }
    default:
        break;
    }
}

// Weak, so that a test linking the real takeover state reads that instead.
__attribute__((weak)) bool UI_Takeover_IsAnyHeld(void)
{
    return m_ScreenHeld;
}

// Weak, so that a test linking the real console reads that instead.
__attribute__((weak)) bool LUA_Console_IsRunningCommand(void)
{
    return false;
}

void SG_Resume_StoreGameToEntry(const GF_LEVEL *const level)
{
}

RESUME_INFO *SG_Resume_GetEntry(const GF_LEVEL *const level)
{
    return level == nullptr ? nullptr : &m_Resume;
}

void FakeGame_SetRunTime(const int32_t frames)
{
    m_Resume.stats.timer = frames;
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
    return m_IsPlaying;
}

void FakeGame_SetPlaying(const bool playing)
{
    m_IsPlaying = playing;
}

double Clock_GetRealTime(void)
{
    return 0.0;
}

bool PhotoMode_IsActive(void)
{
    return m_InPhotoMode;
}

PHOTO_MODE PhotoMode_GetCurrentMode(void)
{
    return m_PhotoModeTarget;
}

void FakeGame_SetPhotoMode(const bool active, const PHOTO_MODE target)
{
    m_InPhotoMode = active;
    m_PhotoModeTarget = target;
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
    lua_pushcfunction(L, M_L_OpenRing);
    lua_setfield(L, -2, "open_ring");
    lua_pushcfunction(L, M_L_CloseRing);
    lua_setfield(L, -2, "close_ring");
    lua_pushcfunction(L, M_L_SettleRing);
    lua_setfield(L, -2, "settle_ring");
    lua_pushcfunction(L, M_L_SetScreenHeld);
    lua_setfield(L, -2, "set_screen_held");
    lua_pushcfunction(L, M_L_SetNGPlus);
    lua_setfield(L, -2, "set_ngplus");
    lua_pushcfunction(L, M_L_SetCurrentLevel);
    lua_setfield(L, -2, "set_current_level");
    lua_pushcfunction(L, M_L_SetCurrentTitle);
    lua_setfield(L, -2, "set_current_title");
    lua_pushcfunction(L, M_L_SetCurrentCutscene);
    lua_setfield(L, -2, "set_current_cutscene");
    lua_pushcfunction(L, M_L_SetInCutscene);
    lua_setfield(L, -2, "set_in_cutscene");
    lua_pushcfunction(L, M_L_SetGymPresent);
    lua_setfield(L, -2, "set_gym_present");
    lua_pushcfunction(L, M_L_SetPhotoMode);
    lua_setfield(L, -2, "set_photo_mode");
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
