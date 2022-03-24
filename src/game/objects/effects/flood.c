#include "game/objects/effects/flood.h"

#include "game/sound.h"
#include "global/vars.h"

void Flood(ITEM_INFO *item)
{
    PHD_3DPOS pos;

    if (g_FlipTimer > FRAMES_PER_SECOND * 4) {
        g_FlipEffect = -1;
    } else {
        pos.x = g_LaraItem->pos.x;
        if (g_FlipTimer < FRAMES_PER_SECOND) {
            pos.y = g_Camera.target.y + (FRAMES_PER_SECOND - g_FlipTimer) * 100;
        } else {
            pos.y = g_Camera.target.y + (g_FlipTimer - FRAMES_PER_SECOND) * 100;
        }
        pos.z = g_LaraItem->pos.z;
        Sound_Effect(SFX_WATERFALL_BIG, &pos, SPM_NORMAL);
    }

    g_FlipTimer++;
}
