#include "game/item_actions/stairs2slope.h"

#include "game/room.h"
#include "game/sound.h"

void ItemAction_Stairs2Slope(ITEM *item)
{
    if (g_FlipTimer == 5) {
        Sound_Effect(SFX_STAIRS_2_SLOPE_FX, nullptr, SPM_NORMAL);
        Room_SetFlipEffect(-1);
    }
    g_FlipTimer++;
}
