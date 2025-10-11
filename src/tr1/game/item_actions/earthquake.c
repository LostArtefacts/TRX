#include "game/item_actions/earthquake.h"

#include <libtrx/game/camera.h>
#include <libtrx/game/rooms.h>
#include <libtrx/game/sound.h>

void ItemAction_Earthquake(ITEM *item)
{
    const int32_t flip_timer = Room_GetFlipTimer();
    if (flip_timer == 0) {
        Sound_Effect(SFX_EXPLOSION_2, nullptr, SPM_NORMAL);
        g_Camera.bounce = -250;
    } else if (flip_timer == 3) {
        Sound_Effect(SFX_ROLLING_BALL, nullptr, SPM_NORMAL);
    } else if (flip_timer == 35) {
        Sound_Effect(SFX_EXPLOSION_2, nullptr, SPM_NORMAL);
    } else if (flip_timer == 20 || flip_timer == 50 || flip_timer == 70) {
        Sound_Effect(SFX_T_REX_STOMP, nullptr, SPM_NORMAL);
    }

    if (flip_timer == 104) {
        Room_SetFlipEffect(-1);
    }
    Room_IncrementFlipTimer(1);
}
