#include "game/stats.h"

#include "game/clock.h"
#include "game/game_flow.h"
#include "global/vars.h"

#define USE_REAL_CLOCK 0

#if USE_REAL_CLOCK
static CLOCK_TIMER m_StartCounter = { .type = CLOCK_TYPE_REAL };
static int32_t m_StartTimer = 0;

void Stats_StartTimer(void)
{
    ClockTimer_Sync(&m_StartCounter);
    m_StartTimer = g_SaveGame.current_stats.timer;
}

void Stats_UpdateTimer(void)
{
    const double elapsed = ClockTimer_PeekElapsed(&m_StartCounter) * LOGIC_FPS;
    g_SaveGame.current_stats.timer = m_StartTimer + elapsed;
}
#else
void Stats_StartTimer(void)
{
}

void Stats_UpdateTimer(void)
{
    g_SaveGame.current_stats.timer++;
}
#endif

FINAL_STATS Stats_ComputeFinalStats(void)
{
    FINAL_STATS result = {};

    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_MAIN);
    for (int32_t i = 0; i < level_table->count; i++) {
        const GF_LEVEL *const level = &level_table->levels[i];
        if (level->type == GFL_GYM) {
            continue;
        }
        result.timer += g_SaveGame.start[i].stats.timer;
        result.ammo_used += g_SaveGame.start[i].stats.ammo_used;
        result.ammo_hits += g_SaveGame.start[i].stats.ammo_hits;
        result.kills += g_SaveGame.start[i].stats.kills;
        result.distance += g_SaveGame.start[i].stats.distance;
        result.medipacks += g_SaveGame.start[i].stats.medipacks;

        // TODO: #170, consult GFE_NUM_SECRETS rather than hardcoding this
        if (i < level_table->count - 2) {
            for (int32_t j = 0; j < 3; j++) {
                if (g_SaveGame.start[i].stats.secret_flags & (1 << j)) {
                    result.found_secrets++;
                }
                result.total_secrets++;
            }
        }
    }

    return result;
}

void Stats_Reset(void)
{
    g_SaveGame.current_stats.timer = 0;
    g_SaveGame.current_stats.kills = 0;
    g_SaveGame.current_stats.distance = 0;
    g_SaveGame.current_stats.ammo_hits = 0;
    g_SaveGame.current_stats.ammo_used = 0;
    g_SaveGame.current_stats.medipacks = 0;
    g_SaveGame.current_stats.secret_flags = 0;
}
