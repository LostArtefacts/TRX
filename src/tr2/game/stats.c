#include "game/stats.h"

#include "game/game.h"
#include "game/game_flow.h"
#include "game/savegame.h"

#include <libtrx/debug.h>
#include <libtrx/game/objects/vars.h>
#include <libtrx/log.h>
#include <libtrx/utils.h>

typedef struct {
    int32_t secret_count;
    uint32_t secret_flags;
    OBJECT_ID secret_objects[STATS_MAX_SECRETS];
} M_MAX_STATS;

static int32_t m_CachedItemCount = 0;
static M_MAX_STATS m_LevelMax = {};

void Stats_UpdateTimer(void)
{
    const GF_LEVEL *const level = Game_GetCurrentLevel();
    if (level != nullptr) {
        RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);
        resume->stats.timer++;
    }
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
        result.kill_count += stats->kill_count;
        result.distance_travelled += stats->distance_travelled;
        result.medipacks_used += stats->medipacks_used;
        result.secret_count += stats->secret_count;
        result.max_secret_count += stats->max_secret_count;
    }

    return result;
}

void Stats_ObserveItemsLoad(void)
{
    m_CachedItemCount = Item_GetLevelCount();
}

void Stats_CalculateStats(void)
{
    LOG_INFO("Recalculating stats");
    m_LevelMax.secret_count = 0;
    m_LevelMax.secret_flags = 0;
    for (int32_t i = 0; i < STATS_MAX_SECRETS; i++) {
        m_LevelMax.secret_objects[i] = NO_OBJECT;
    }

    struct L_SECRET_ITEM {
        OBJECT_ID object_id;
        int32_t index;
    } secrets[STATS_MAX_SECRETS];
    int32_t secret_count = 0;
    for (int32_t i = 0; i < m_CachedItemCount; i++) {
        ITEM *const item = Item_Get(i);
        if (item->object_id < O_FIRST || item->object_id >= O_NUMBER_OF) {
            LOG_ERROR("Bad Object number (%d) on Item %d", item->object_id, i);
            continue;
        }
        if (Object_IsType(item->object_id, g_SecretObjects)) {
            if (secret_count >= STATS_MAX_SECRETS) {
                LOG_ERROR("Too many secrets, max %d", STATS_MAX_SECRETS);
                break;
            }
            secrets[secret_count].object_id = item->object_id;
            secrets[secret_count].index = i;
            secret_count++;
        }
    }

    for (int32_t i = 0; i < secret_count; i++) {
        for (int32_t j = i + 1; j < secret_count; j++) {
            if (secrets[i].object_id > secrets[j].object_id) {
                SWAP(secrets[i], secrets[j]);
            }
        }
    }

    for (int32_t k = 0; k < secret_count; k++) {
        ITEM *const item = Item_Get(secrets[k].index);
        item->data =
            (void *)(intptr_t)Stats_ReserveSecretBit(secrets[k].object_id);
    }
}

uint32_t Stats_ReserveSecretBit(const OBJECT_ID object_id)
{
    uint32_t n = m_LevelMax.secret_flags;
    int32_t position = 0;
    while ((n & 1) == 1) {
        n >>= 1;
        position++;
    }
    LOG_INFO("Reserving bit %d for secret %d", position, object_id);
    m_LevelMax.secret_flags |= 1 << position;
    m_LevelMax.secret_count++;
    m_LevelMax.secret_objects[position] = object_id;
    return 1 << position;
}

OBJECT_ID Stats_GetSecretObject(const int32_t secret_idx)
{
    ASSERT(secret_idx >= 0 && secret_idx < STATS_MAX_SECRETS);
    return m_LevelMax.secret_objects[secret_idx];
}

int32_t Stats_GetMaxSecrets(void)
{
    return m_LevelMax.secret_count;
}

uint32_t Stats_GetMaxSecretFlags(void)
{
    return m_LevelMax.secret_flags;
}

void Stats_MarkSecretCollected(const ITEM *const item)
{
    RESUME_INFO *const resume = Savegame_GetCurrentInfo(Game_GetCurrentLevel());
    resume->stats.secret_flags |= (uint32_t)(intptr_t)item->data;
    Stats_UpdateSecrets(&resume->stats);
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
    return stats.secret_count >= stats.max_secret_count;
}

void Stats_AddKill(void)
{
    RESUME_INFO *const current_info =
        Savegame_GetCurrentInfo(Game_GetCurrentLevel());
    current_info->stats.kill_count++;
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
