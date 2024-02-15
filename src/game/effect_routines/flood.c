#include "game/effect_routines/flood.h"

#include "game/room.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/vars.h"

#include <stdint.h>

void FX_Flood(ITEM_INFO *item)
{
    if (g_FlipTimer > FRAMES_PER_SECOND * 4) {
        g_FlipEffect = -1;
    } else {
        const int32_t timer = g_FlipTimer < FRAMES_PER_SECOND
            ? FRAMES_PER_SECOND - g_FlipTimer
            : g_FlipTimer - FRAMES_PER_SECOND;
        const VECTOR_3D pos = {
            .x = g_LaraItem->pos.x,
            .y = g_Camera.target.y + timer * 100,
            .z = g_LaraItem->pos.z,
        };
        Sound_Effect(SFX_WATERFALL_BIG, &pos, SPM_NORMAL);
    }

    g_FlipTimer++;
}
