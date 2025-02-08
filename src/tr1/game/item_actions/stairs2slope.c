#include "game/item_actions/stairs2slope.h"

#include "game/room.h"
#include "game/sound.h"

void ItemAction_Stairs2Slope(ITEM *item)
{
    int32_t flip_timer = Room_GetFlipTimer();
    if (flip_timer == 5) {
        Sound_Effect(SFX_STAIRS_2_SLOPE_FX, nullptr, SPM_NORMAL);
        Room_SetFlipEffect(-1);
    }
    Room_SetFlipTimer(++flip_timer);
}
