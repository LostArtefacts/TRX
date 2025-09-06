#include "game/item_actions/stairs2slope.h"

#include <libtrx/game/rooms.h>
#include <libtrx/game/sound.h>

void ItemAction_Stairs2Slope(ITEM *item)
{
    const int32_t flip_timer = Room_GetFlipTimer();
    if (flip_timer == 5) {
        Sound_Effect(SFX_STAIRS_2_SLOPE_FX, nullptr, SPM_NORMAL);
        Room_SetFlipEffect(-1);
    }
    Room_IncrementFlipTimer(1);
}
