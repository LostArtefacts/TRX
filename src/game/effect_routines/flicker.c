#include "game/effect_routines/flicker.h"

#include "game/room.h"
#include "game/sound.h"

void FX_Flicker(ITEM_INFO *item)
{
    if (g_FlipTimer > 125) {
        Room_FlipMap();
        g_FlipEffect = -1;
    } else if (
        g_FlipTimer == 90 || g_FlipTimer == 92 || g_FlipTimer == 105
        || g_FlipTimer == 107) {
        Room_FlipMap();
    }
    g_FlipTimer++;
}
