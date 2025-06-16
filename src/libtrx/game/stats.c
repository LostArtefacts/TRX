#include "game/stats.h"

#include "game/game.h"
#include "game/savegame.h"

bool Stats_HasSecret(const int16_t secret_num)
{
    RESUME_INFO *const current_info =
        Savegame_GetCurrentInfo(Game_GetCurrentLevel());
    if (secret_num < 0 || secret_num >= current_info->stats.max_secret_count) {
        return false;
    }
    return (current_info->stats.secret_flags & (1 << secret_num)) != 0;
}

bool Stats_TakeSecret(const int16_t secret_num)
{
    RESUME_INFO *const current_info =
        Savegame_GetCurrentInfo(Game_GetCurrentLevel());
    if (secret_num < 0 || secret_num >= current_info->stats.max_secret_count) {
        return false;
    }
    if (!(current_info->stats.secret_flags & (1 << secret_num))) {
        return false;
    }
    current_info->stats.secret_flags &= ~(1 << secret_num);
// TODO: support this in TR2
#if TR_VERSION == 1
    current_info->stats.secret_count--;
#endif
    return true;
}

bool Stats_AddSecret(const int16_t secret_num)
{
    RESUME_INFO *const current_info =
        Savegame_GetCurrentInfo(Game_GetCurrentLevel());
    if (secret_num < 0 || secret_num >= current_info->stats.max_secret_count) {
        return false;
    }
    if (current_info->stats.secret_flags & (1 << secret_num)) {
        return false;
    }
    current_info->stats.secret_flags |= 1 << secret_num;
// TODO: support this in TR2
#if TR_VERSION == 1
    current_info->stats.secret_count++;
#endif
    return true;
}
