#include "game/stats.h"

#include "game/clock.h"
#include "game/game_flow.h"
#include "game/objects/vars.h"
#include "global/vars.h"

#include <libtrx/log.h>

#define USE_REAL_CLOCK 0

static int32_t m_CachedItemCount = 0;
static int32_t m_LevelSecrets = 0;

static bool M_SetSecretFlag(uint8_t *flags, GAME_OBJECT_ID obj_id);

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

static bool M_SetSecretFlag(uint8_t *const flags, const GAME_OBJECT_ID obj_id)
{
    for (int32_t i = 0; i < 2; i++) {
        const int32_t flag = 1 << ((obj_id - O_SECRET_1) + i * 3);
        if ((*flags & flag) == 0) {
            *flags |= flag;
            return true;
        }
    }

    return false;
}

FINAL_STATS Stats_ComputeFinalStats(GF_LEVEL_TYPE level_type)
{
    FINAL_STATS result = {};

    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_MAIN);
    for (int32_t i = 0; i < level_table->count; i++) {
        const GF_LEVEL *const level = &level_table->levels[i];
        if (level->type != level_type) {
            continue;
        }
        result.timer += g_SaveGame.start[i].stats.timer;
        result.ammo_used += g_SaveGame.start[i].stats.ammo_used;
        result.ammo_hits += g_SaveGame.start[i].stats.ammo_hits;
        result.kills += g_SaveGame.start[i].stats.kills;
        result.distance += g_SaveGame.start[i].stats.distance;
        result.medipacks += g_SaveGame.start[i].stats.medipacks;

        for (int32_t j = 0; j < g_SaveGame.start[i].stats.max_secret_count;
             j++) {
            if (g_SaveGame.start[i].stats.secret_flags & (1 << j)) {
                result.found_secrets++;
            }
            result.total_secrets++;
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

void Stats_ObserveItemsLoad(void)
{
    m_CachedItemCount = Item_GetLevelCount();
}

void Stats_CalculateStats(void)
{
    m_LevelSecrets = 0;
    uint8_t secret_flags = 0;

    for (int32_t i = 0; i < m_CachedItemCount; i++) {
        const ITEM *const item = Item_Get(i);
        if (item->object_id < 0 || item->object_id >= O_NUMBER_OF) {
            LOG_ERROR("Bad Object number (%d) on Item %d", item->object_id, i);
            continue;
        }

        if (Object_IsType(item->object_id, g_SecretObjects)
            && M_SetSecretFlag(&secret_flags, item->object_id)) {
            m_LevelSecrets++;
        }
    }
}

int32_t Stats_GetSecrets(void)
{
    return m_LevelSecrets;
}

void Stats_MarkSecretCollected(const GAME_OBJECT_ID obj_id)
{
    M_SetSecretFlag(&g_SaveGame.current_stats.secret_flags, obj_id);
}

bool Stats_CheckAllLevelSecretsCollected(void)
{
    int32_t flags = g_SaveGame.current_stats.secret_flags;
    int32_t count = 0;
    while (flags != 0) {
        count += flags & 1;
        flags >>= 1;
    }

    return count >= g_SaveGame.current_stats.max_secret_count;
}

bool Stats_CheckAllSecretsCollected(GF_LEVEL_TYPE level_type)
{
    const FINAL_STATS stats = Stats_ComputeFinalStats(level_type);
    return stats.found_secrets >= stats.total_secrets;
}
