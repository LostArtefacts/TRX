#include "game/objects/effects/dino_stomp.h"

#include "global/vars.h"

#define MAX_BOUNCE 100

void DinoStomp(ITEM_INFO *item)
{
    int32_t dx = item->pos.x - g_Camera.pos.x;
    int32_t dy = item->pos.y - g_Camera.pos.y;
    int32_t dz = item->pos.z - g_Camera.pos.z;
    int32_t limit = 16 * WALL_L;
    if (ABS(dx) < limit && ABS(dy) < limit && ABS(dz) < limit) {
        int32_t dist = (SQUARE(dx) + SQUARE(dy) + SQUARE(dz)) / 256;
        g_Camera.bounce =
            ((SQUARE(WALL_L) - dist) * MAX_BOUNCE) / SQUARE(WALL_L);
    }
}
