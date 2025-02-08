#include "game/item_actions/flicker.h"

#include "game/room.h"

void ItemAction_Flicker(ITEM *item)
{
    if (g_FlipTimer > 125) {
        Room_FlipMap();
        Room_SetFlipEffect(-1);
    } else if (
        g_FlipTimer == 90 || g_FlipTimer == 92 || g_FlipTimer == 105
        || g_FlipTimer == 107) {
        Room_FlipMap();
    }
    g_FlipTimer++;
}
