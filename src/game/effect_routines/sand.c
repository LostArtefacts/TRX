#include "game/effect_routines/sand.h"

#include "game/room.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/vars.h"

#include <stddef.h>

void FX_DropSand(ITEM_INFO *item)
{
    if (g_FlipTimer > FRAMES_PER_SECOND * 4) {
        g_FlipEffect = -1;
    } else {
        if (!g_FlipTimer) {
            Sound_Effect(SFX_TRAPDOOR_OPEN, NULL, SPM_NORMAL);
        }
        const XYZ_32 pos = {
            .x = g_Camera.target.x,
            .y = g_Camera.target.y + g_FlipTimer * 100,
            .z = g_Camera.target.z,
        };
        Sound_Effect(SFX_SAND_FX, &pos, SPM_NORMAL);
    }
    g_FlipTimer++;
}
