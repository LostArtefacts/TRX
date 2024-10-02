#include "game/effect_routines/stairs2slope.h"

#include "game/room.h"
#include "game/sound.h"

#include <stddef.h>

void FX_Stairs2Slope(ITEM *item)
{
    if (g_FlipTimer == 5) {
        Sound_Effect(SFX_STAIRS2SLOPE_FX, NULL, SPM_NORMAL);
        g_FlipEffect = -1;
    }
    g_FlipTimer++;
}
