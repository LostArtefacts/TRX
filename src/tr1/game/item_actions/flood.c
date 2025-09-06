#include "game/item_actions/flood.h"

#include <libtrx/game/camera.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/rooms.h>
#include <libtrx/game/sound.h>

void ItemAction_Flood(ITEM *item)
{
    const int32_t flip_timer = Room_GetFlipTimer();
    if (flip_timer > LOGIC_FPS * 4) {
        Room_SetFlipEffect(-1);
    } else {
        const ITEM *const lara_item = Lara_GetItem();
        const int32_t timer = flip_timer < LOGIC_FPS ? LOGIC_FPS - flip_timer
                                                     : flip_timer - LOGIC_FPS;
        const XYZ_32 pos = {
            .x = lara_item->pos.x,
            .y = g_Camera.target.y + timer * 100,
            .z = lara_item->pos.z,
        };
        Sound_Effect(SFX_WATERFALL_BIG, &pos, SPM_NORMAL);
    }

    Room_IncrementFlipTimer(1);
}
