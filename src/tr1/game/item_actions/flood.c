#include "game/item_actions/flood.h"

#include "game/camera.h"
#include "game/room.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/vars.h"

#include <stdint.h>

void ItemAction_Flood(ITEM *item)
{
    const int32_t flip_timer = Room_GetFlipTimer();
    if (flip_timer > LOGIC_FPS * 4) {
        Room_SetFlipEffect(-1);
    } else {
        const int32_t timer = flip_timer < LOGIC_FPS ? LOGIC_FPS - flip_timer
                                                     : flip_timer - LOGIC_FPS;
        const XYZ_32 pos = {
            .x = g_LaraItem->pos.x,
            .y = g_Camera.target.y + timer * 100,
            .z = g_LaraItem->pos.z,
        };
        Sound_Effect(SFX_WATERFALL_BIG, &pos, SPM_NORMAL);
    }

    Room_IncrementFlipTimer(1);
}
