#include "game/effects/sand.h"

#include "game/sound.h"
#include "global/vars.h"

void DropSand(ITEM_INFO *item)
{
    PHD_3DPOS pos;
    if (g_FlipTimer > FRAMES_PER_SECOND * 4) {
        g_FlipEffect = -1;
    } else {
        if (!g_FlipTimer) {
            Sound_Effect(SFX_TRAPDOOR_OPEN, NULL, SPM_NORMAL);
        }
        pos.x = g_Camera.target.x;
        pos.y = g_Camera.target.y + g_FlipTimer * 100;
        pos.z = g_Camera.target.z;
        Sound_Effect(SFX_SAND_FX, &pos, SPM_NORMAL);
    }
    g_FlipTimer++;
}
