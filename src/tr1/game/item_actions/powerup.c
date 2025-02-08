#include "game/item_actions/powerup.h"

#include "game/camera.h"
#include "game/room.h"
#include "game/sound.h"
#include "global/const.h"

void ItemAction_PowerUp(ITEM *item)
{
    if (g_FlipTimer > LOGIC_FPS * 4) {
        Room_SetFlipEffect(-1);
    } else {
        const XYZ_32 pos = {
            .x = g_Camera.target.x,
            .y = g_Camera.target.y + g_FlipTimer * 100,
            .z = g_Camera.target.z,
        };
        Sound_Effect(SFX_POWERUP_FX, &pos, SPM_NORMAL);
    }
    g_FlipTimer++;
}
