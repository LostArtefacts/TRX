#include "game/item_actions/flicker.h"

#include "game/room.h"

void ItemAction_Flicker(ITEM *item)
{
    int32_t flip_timer = Room_GetFlipTimer();
    if (flip_timer > 125) {
        Room_FlipMap();
        Room_SetFlipEffect(-1);
    } else if (
        flip_timer == 90 || flip_timer == 92 || flip_timer == 105
        || flip_timer == 107) {
        Room_FlipMap();
    }
    Room_SetFlipTimer(++flip_timer);
}
