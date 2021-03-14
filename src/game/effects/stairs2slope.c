#include "game/effects/stairs2slope.h"
#include "game/sound.h"
#include "game/vars.h"

// original name: Stairs2SlopeFX
void Stairs2Slope(ITEM_INFO *item)
{
    if (FlipTimer == 5) {
        SoundEffect(SFX_STAIRS2SLOPE_FX, NULL, SPM_NORMAL);
        FlipEffect = -1;
    }
    FlipTimer++;
}
