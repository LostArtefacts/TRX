#include "game/item_actions/earthquake.h"

#include "game/camera.h"
#include "game/room.h"
#include "game/sound.h"

void ItemAction_Earthquake(ITEM *item)
{
    if (g_FlipTimer == 0) {
        Sound_Effect(SFX_EXPLOSION, nullptr, SPM_NORMAL);
        g_Camera.bounce = -250;
    } else if (g_FlipTimer == 3) {
        Sound_Effect(SFX_ROLLING_BALL, nullptr, SPM_NORMAL);
    } else if (g_FlipTimer == 35) {
        Sound_Effect(SFX_EXPLOSION, nullptr, SPM_NORMAL);
    } else if (g_FlipTimer == 20 || g_FlipTimer == 50 || g_FlipTimer == 70) {
        Sound_Effect(SFX_T_REX_FOOTSTOMP, nullptr, SPM_NORMAL);
    }

    g_FlipTimer++;
    if (g_FlipTimer == 105) {
        Room_SetFlipEffect(-1);
    }
}
