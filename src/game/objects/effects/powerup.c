#include "game/objects/effects/powerup.h"

#include "game/sound.h"
#include "global/vars.h"

void PowerUp(ITEM_INFO *item)
{
    PHD_3DPOS pos;
    if (g_FlipTimer > FRAMES_PER_SECOND * 4) {
        g_FlipEffect = -1;
    } else {
        pos.x = g_Camera.target.x;
        pos.y = g_Camera.target.y + g_FlipTimer * 100;
        pos.z = g_Camera.target.z;
        Sound_Effect(SFX_POWERUP_FX, &pos, SPM_NORMAL);
    }
    g_FlipTimer++;
}
