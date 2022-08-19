#include "game/effect_routines/earthquake.h"

#include "game/room.h"
#include "game/sound.h"
#include "global/vars.h"

void FX_Earthquake(ITEM_INFO *item)
{
    if (g_FlipTimer == 0) {
        Sound_Effect(SFX_EXPLOSION, NULL, SPM_NORMAL);
        g_Camera.bounce = -250;
    } else if (g_FlipTimer == 3) {
        Sound_Effect(SFX_ROLLING_BALL, NULL, SPM_NORMAL);
    } else if (g_FlipTimer == 35) {
        Sound_Effect(SFX_EXPLOSION, NULL, SPM_NORMAL);
    } else if (g_FlipTimer == 20 || g_FlipTimer == 50 || g_FlipTimer == 70) {
        Sound_Effect(SFX_T_REX_FOOTSTOMP, NULL, SPM_NORMAL);
    }

    g_FlipTimer++;
    if (g_FlipTimer == 105) {
        g_FlipEffect = -1;
    }
}
