#include "game/effect_routines/flood.h"

#include "game/room.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/vars.h"

#include <stdint.h>

void FX_Flood(ITEM_INFO *item)
{
    if (g_FlipTimer > LOGIC_FPS * 4) {
        g_FlipEffect = -1;
    } else {
        const int32_t timer = g_FlipTimer < LOGIC_FPS ? LOGIC_FPS - g_FlipTimer
                                                      : g_FlipTimer - LOGIC_FPS;
        const XYZ_32 pos = {
            .x = g_LaraItem->pos.x,
            .y = g_Camera.target.y + timer * 100,
            .z = g_LaraItem->pos.z,
        };
        Sound_Effect(SFX_WATERFALL_BIG, &pos, SPM_NORMAL);
    }

    g_FlipTimer++;
}
