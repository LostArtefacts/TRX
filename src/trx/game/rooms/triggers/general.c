#include <trx/config.h>
#include <trx/game/game.h>
#include <trx/game/items.h>
#include <trx/game/music.h>
#include <trx/game/rooms.h>
#include <trx/game/stats.h>

static void M_HandleBodyBag(
    const TRIGGER *const trigger, const TRIGGER_CMD *const cmd,
    TRIGGER_STATUS *const status)
{
    if (g_Config.gameplay.enable_body_bags) {
        Item_ClearKilled();
    }
}

static void M_HandleSecret(
    const TRIGGER *const trigger, const TRIGGER_CMD *const cmd,
    TRIGGER_STATUS *const status)
{
    const int16_t secret_num = (int16_t)(intptr_t)cmd->parameter;
    if (Stats_AddSecret(secret_num)) {
        const MUSIC_PLAY_MODE mode =
            g_Config.audio.fix_secrets_killing_music ? MPM_OVERLAY : MPM_ONCE;
        Music_Play(MX_SECRET, mode);
    }
}

static void M_HandleFinish(
    const TRIGGER *const trigger, const TRIGGER_CMD *const cmd,
    TRIGGER_STATUS *const status)
{
    Game_SetIsLevelComplete(true);
}

REGISTER_TRIGGER_HANDLER(TO_BODY_BAG, M_HandleBodyBag)
REGISTER_TRIGGER_HANDLER(TO_SECRET, M_HandleSecret)
REGISTER_TRIGGER_HANDLER(TO_FINISH, M_HandleFinish)
