#include "game/stats.h"

#include "game/game.h"
#include "game/savegame.h"

bool Stats_HasSecret(const int16_t secret_num)
{
    RESUME_INFO *const current_info =
        Savegame_GetCurrentInfo(Game_GetCurrentLevel());
    if (secret_num < 0 || secret_num >= STATS_MAX_SECRETS) {
        return false;
    }
    const uint32_t secret_mask = 1 << secret_num;
    if ((secret_mask & current_info->stats.all_secrets_mask) == 0) {
        return false;
    }
    return (current_info->stats.secret_flags & secret_mask) != 0;
}

bool Stats_TakeSecret(const int16_t secret_num)
{
    RESUME_INFO *const current_info =
        Savegame_GetCurrentInfo(Game_GetCurrentLevel());
    if (secret_num < 0 || secret_num >= STATS_MAX_SECRETS) {
        return false;
    }
    const uint32_t secret_mask = 1 << secret_num;
    if ((secret_mask & current_info->stats.all_secrets_mask) == 0) {
        return false;
    }
    if (!(current_info->stats.secret_flags & secret_mask)) {
        return false;
    }
    current_info->stats.secret_flags &= ~secret_mask;
    current_info->stats.secret_count--;
    return true;
}

bool Stats_AddSecret(const int16_t secret_num)
{
    RESUME_INFO *const current_info =
        Savegame_GetCurrentInfo(Game_GetCurrentLevel());
    if (secret_num < 0 || secret_num >= STATS_MAX_SECRETS) {
        return false;
    }
    const uint32_t secret_mask = 1 << secret_num;
    if ((secret_mask & current_info->stats.all_secrets_mask) == 0) {
        return false;
    }
    if (current_info->stats.secret_flags & secret_mask) {
        return false;
    }
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
