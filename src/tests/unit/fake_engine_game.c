// A game flow of three levels, one cutscene and one demo. The second level is a
// gym, which is the case worth having: a gym has no number, so the ordinal a
// script reads is not the level's place in the table.

#include "fake_engine_game.h"

#include <trx/game/game_flow/common.h>
#include <trx/game/savegame.h>

#include <stdbool.h>
#include <string.h>

FAKE_GAME_CALLS g_FakeGameCalls;

static GF_LEVEL m_Levels[FAKE_LEVEL_COUNT];
static GF_LEVEL m_Cutscenes[FAKE_CUTSCENE_COUNT];
static GF_LEVEL m_Demos[FAKE_DEMO_COUNT];
static GF_LEVEL_TABLE m_Tables[GFLT_NUMBER_OF];
static const GF_LEVEL *m_CurrentLevel;

const GF_LEVEL_TABLE *GF_GetLevelTable(const GF_LEVEL_TABLE_TYPE table_type)
{
    if (table_type < 0 || table_type >= GFLT_NUMBER_OF) {
        return nullptr;
    }
    return &m_Tables[table_type];
}

int32_t GF_GetLevelCount(const GF_LEVEL_TABLE_TYPE table_type)
{
    const GF_LEVEL_TABLE *const tbl = GF_GetLevelTable(table_type);
    return tbl != nullptr ? tbl->count : 0;
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

const GF_LEVEL *GF_GetCurrentLevel(void)
{
    return m_CurrentLevel;
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

// A gym has no number, and everything after it still counts up.
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

// The gameflow command the play_* verbs queue, and the savegame bookkeeping
// they do on the way. Recorded, not performed.
void GF_OverrideCommand(const GF_COMMAND command)
{
    switch (command.action) {
    case GF_START_GAME:
        g_FakeGameCalls.play_level++;
        break;
    case GF_START_CINE:
        g_FakeGameCalls.play_cutscene++;
        break;
    case GF_START_DEMO:
        g_FakeGameCalls.play_demo++;
        break;
    default:
        break;
    }
    g_FakeGameCalls.last_num = command.param;
}

void Savegame_PersistGameToCurrentInfo(const GF_LEVEL *const level)
{
}

RESUME_INFO *Savegame_GetCurrentInfo(const GF_LEVEL *const level)
{
    return nullptr;
}

int32_t g_TRVersion = 1;
const char *g_TRXVersion = "TRX-test";

void FakeGame_Reset(void)
{
    memset(m_Levels, 0, sizeof(m_Levels));
    memset(m_Cutscenes, 0, sizeof(m_Cutscenes));
    memset(m_Demos, 0, sizeof(m_Demos));

    m_Levels[0] = (GF_LEVEL) {
        .num = 0,
        .type = GFL_GYM,
        .title = "Lara's Home",
        .path = "gym.phd",
        .lara_outfit = "casual",
        .water_particles = false,
    };
    m_Levels[1] = (GF_LEVEL) {
        .num = 1,
        .type = GFL_NORMAL,
        .title = "Caves",
        .path = "level1.phd",
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

    m_Tables[GFLT_MAIN] =
        (GF_LEVEL_TABLE) { .count = FAKE_LEVEL_COUNT, .levels = m_Levels };
    m_Tables[GFLT_CUTSCENES] = (GF_LEVEL_TABLE) { .count = FAKE_CUTSCENE_COUNT,
                                                  .levels = m_Cutscenes };
    m_Tables[GFLT_DEMOS] =
        (GF_LEVEL_TABLE) { .count = FAKE_DEMO_COUNT, .levels = m_Demos };
    m_Tables[GFLT_TITLE] = (GF_LEVEL_TABLE) { .count = 0, .levels = nullptr };

    m_CurrentLevel = nullptr;
    g_FakeGameCalls = (FAKE_GAME_CALLS) {};
}

void FakeGame_SetCurrentLevel(const int32_t idx)
{
    m_CurrentLevel = idx < 0 ? nullptr : &m_Levels[idx];
}
