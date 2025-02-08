#include "game/item_actions/sand.h"

#include "game/camera.h"
#include "game/room.h"
#include "game/sound.h"
#include "global/const.h"

void ItemAction_DropSand(ITEM *item)
{
    int32_t flip_timer = Room_GetFlipTimer();
    if (flip_timer > LOGIC_FPS * 4) {
        Room_SetFlipEffect(-1);
    } else {
        if (flip_timer == 0) {
            Sound_Effect(SFX_TRAPDOOR_OPEN, nullptr, SPM_NORMAL);
        }
        const XYZ_32 pos = {
            .x = g_Camera.target.x,
            .y = g_Camera.target.y + flip_timer * 100,
            .z = g_Camera.target.z,
        };
        Sound_Effect(SFX_SAND_FX, &pos, SPM_NORMAL);
    }
    Room_SetFlipTimer(++flip_timer);
}
