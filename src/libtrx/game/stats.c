#include "game/stats.h"

#include "game/game.h"
#include "game/savegame.h"
#include "log.h"
#include "utils.h"

bool Stats_IsSecretValid(const int16_t secret_idx)
{
    RESUME_INFO *const current_info =
        Savegame_GetCurrentInfo(Game_GetCurrentLevel());
    if (secret_idx < 0 || secret_idx >= STATS_MAX_SECRETS) {
        return false;
    }
    const uint32_t secret_mask = 1 << secret_idx;
    return (secret_mask & current_info->stats.all_secrets_mask) != 0;
}

bool Stats_HasSecret(const int16_t secret_idx)
{
    RESUME_INFO *const current_info =
        Savegame_GetCurrentInfo(Game_GetCurrentLevel());
    if (secret_idx < 0 || secret_idx >= STATS_MAX_SECRETS) {
        return false;
    }
    const uint32_t secret_mask = 1 << secret_idx;
    if ((secret_mask & current_info->stats.all_secrets_mask) == 0) {
        return false;
    }
    return (current_info->stats.secret_flags & secret_mask) != 0;
}

bool Stats_TakeSecret(const int16_t secret_idx)
{
    RESUME_INFO *const current_info =
        Savegame_GetCurrentInfo(Game_GetCurrentLevel());
    if (secret_idx < 0 || secret_idx >= STATS_MAX_SECRETS) {
        return false;
    }
    const uint32_t secret_mask = 1 << secret_idx;
    if ((secret_mask & current_info->stats.all_secrets_mask) == 0) {
        return false;
    }
    if (!(current_info->stats.secret_flags & secret_mask)) {
        return false;
    }
    LOG_INFO("Removing secret %d", secret_idx);
    current_info->stats.secret_flags &= ~secret_mask;
    current_info->stats.secret_count--;
    return true;
}

bool Stats_AddSecret(const int16_t secret_idx)
{
    RESUME_INFO *const current_info =
        Savegame_GetCurrentInfo(Game_GetCurrentLevel());
    if (secret_idx < 0 || secret_idx >= STATS_MAX_SECRETS) {
        return false;
    }
    const uint32_t secret_mask = 1 << secret_idx;
    if ((secret_mask & current_info->stats.all_secrets_mask) == 0) {
        return false;
    }
    if (current_info->stats.secret_flags & secret_mask) {
        return false;
    }
    LOG_INFO("Adding secret %d", secret_idx);
    current_info->stats.secret_flags |= secret_mask;
    current_info->stats.secret_count++;
    return true;
}

void Stats_UpdateSecrets(LEVEL_STATS *const stats)
{
    stats->secret_count = 0;
    for (int32_t i = 0; i < STATS_MAX_SECRETS; i++) {
        stats->secret_count += (stats->secret_flags & (1 << i)) ? 1 : 0;
    }
}

void Stats_AddMedipacksUsed(const double medipack_value)
{
    RESUME_INFO *const current_info =
        Savegame_GetCurrentInfo(Game_GetCurrentLevel());
    current_info->stats.medipacks_used += medipack_value;
}

void Stats_AddDeath(void)
{
#if TR_VERSION == 1
    const GF_LEVEL *const current_level = Game_GetCurrentLevel();
    RESUME_INFO *const current_info = Savegame_GetCurrentInfo(current_level);
    current_info->stats.death_count++;
    const int32_t save_slot = Savegame_GetBoundSlot();
    if (save_slot != -1) {
        Savegame_UpdateDeathCounters(
            save_slot, current_info->stats.death_count);
    }
#endif
}

void Stats_AddDistanceTravelled(const XYZ_32 pos, const XYZ_32 last_pos)
{
    RESUME_INFO *const current_info =
        Savegame_GetCurrentInfo(Game_GetCurrentLevel());
    current_info->stats.distance_travelled += Math_Sqrt(
        SQUARE(pos.z - last_pos.z) + SQUARE(pos.y - last_pos.y)
        + SQUARE(pos.x - last_pos.x));
}
