#include "game/stats.h"

#include "decomp/savegame.h"
#include "game/clock.h"
#include "game/game.h"
#include "game/game_flow.h"
#include "game/objects/vars.h"
#include "global/vars.h"

#include <libtrx/log.h>
#include <libtrx/utils.h>

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
    const RESUME_INFO *const resume =
        Savegame_GetCurrentInfo(Game_GetCurrentLevel());
    m_StartTimer = resume->stats.timer;
}

void Stats_UpdateTimer(void)
{
    const double elapsed = ClockTimer_PeekElapsed(&m_StartCounter) * LOGIC_FPS;
    RESUME_INFO *const resume = Savegame_GetCurrentInfo(Game_GetCurrentLevel());
    resume->stats.timer = m_StartTimer + elapsed;
}
#else
void Stats_StartTimer(void)
{
}

void Stats_UpdateTimer(void)
{
    const GF_LEVEL *const level = Game_GetCurrentLevel();
    if (level != nullptr) {
        RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);
        resume->stats.timer++;
    }
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

FINAL_STATS Stats_ComputeFinalStats(const GF_LEVEL_TYPE level_type)
{
    FINAL_STATS result = {};

    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_MAIN);
    for (int32_t i = 0; i < level_table->count; i++) {
        const GF_LEVEL *const level = &level_table->levels[i];
        if (level->type != level_type) {
            continue;
        }

        const RESUME_INFO *const info = Savegame_GetCurrentInfo(level);
        const LEVEL_STATS *const stats = &info->stats;
        result.timer += stats->timer;
        result.ammo_used += stats->ammo_used;
        result.ammo_hits += stats->ammo_hits;
        result.kills += stats->kills;
        result.distance += stats->distance;
        result.medipacks += stats->medipacks;

        for (int32_t j = 0; j < stats->max_secret_count; j++) {
            if (stats->secret_flags & (1 << j)) {
                result.found_secrets++;
            }
            result.total_secrets++;
        }
    }

    return result;
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
    RESUME_INFO *const resume = Savegame_GetCurrentInfo(Game_GetCurrentLevel());
    M_SetSecretFlag(&resume->stats.secret_flags, obj_id);
}

bool Stats_CheckAllLevelSecretsCollected(void)
{
    const RESUME_INFO *const resume =
        Savegame_GetCurrentInfo(Game_GetCurrentLevel());
    int32_t flags = resume->stats.secret_flags;
    int32_t count = 0;
    while (flags != 0) {
        count += flags & 1;
        flags >>= 1;
    }

    return count >= resume->stats.max_secret_count;
}

bool Stats_CheckAllSecretsCollected(GF_LEVEL_TYPE level_type)
{
    const FINAL_STATS stats = Stats_ComputeFinalStats(level_type);
    return stats.found_secrets >= stats.total_secrets;
}

void Stats_AddKill(void)
{
    RESUME_INFO *const current_info =
        Savegame_GetCurrentInfo(Game_GetCurrentLevel());
    current_info->stats.kills++;
}

void Stats_AddAmmoHits(void)
{
    RESUME_INFO *const current_info =
        Savegame_GetCurrentInfo(Game_GetCurrentLevel());
    current_info->stats.ammo_hits++;
}

void Stats_AddAmmoUsed(void)
{
    RESUME_INFO *const current_info =
        Savegame_GetCurrentInfo(Game_GetCurrentLevel());
    current_info->stats.ammo_used++;
}

void Stats_AddMedipacksUsed(const double medipack_value)
{
    RESUME_INFO *const current_info =
        Savegame_GetCurrentInfo(Game_GetCurrentLevel());
    current_info->stats.medipacks += medipack_value;
}

void Stats_AddDistanceTravelled(const XYZ_32 pos, const XYZ_32 last_pos)
{
    RESUME_INFO *const current_info =
        Savegame_GetCurrentInfo(Game_GetCurrentLevel());
    current_info->stats.distance += Math_Sqrt(
        SQUARE(pos.z - last_pos.z) + SQUARE(pos.y - last_pos.y)
        + SQUARE(pos.x - last_pos.x));
}
