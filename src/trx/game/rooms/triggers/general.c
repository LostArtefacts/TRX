#include <trx/config.h>
#include <trx/game/game.h>
#include <trx/game/hub.h>
#include <trx/game/items.h>
#include <trx/game/music.h>
#include <trx/game/objects.h>
#include <trx/game/rooms.h>
#include <trx/game/stats.h>

// Remove the bodies of items marked to be cleared once dead. Part of the OG
// performance work, generously used in Opera House and Barkhang Monastery.
static void M_DestroyKilledBodies(void)
{
    for (int32_t i = 0; i < Item_GetLevelCount(); i++) {
        ITEM *const item = Item_Get(i);
        const OBJECT *const obj = Object_Get(item->object_id);
        if (obj->leaves_corpse && item->clear_body && item->hit_points <= 0
            && !item->is_destroyed) {
            Item_Destroy(i);
        }
    }
}

static void M_HandleBodyBag(
    const TRIGGER *const trigger, const TRIGGER_CMD *const cmd,
    TRIGGER_STATUS *const status)
{
    if (g_Config.gameplay.enable_body_bags) {
        M_DestroyKilledBodies();
    }
}

static void M_HandleSecret(
    const TRIGGER *const trigger, const TRIGGER_CMD *const cmd,
    TRIGGER_STATUS *const status)
{
    const int16_t secret_num = (int16_t)(intptr_t)cmd->parameter;
    if (Stats_AddSecret(Game_GetCurrentLevel(), secret_num)) {
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
    Hub_SetNextLevelIndex((int16_t)(intptr_t)cmd->parameter);
    Hub_SetLaraStartIndex(trigger->timer);
}

REGISTER_TRIGGER_HANDLER(TO_BODY_BAG, M_HandleBodyBag)
REGISTER_TRIGGER_HANDLER(TO_SECRET, M_HandleSecret)
REGISTER_TRIGGER_HANDLER(TO_FINISH, M_HandleFinish)
