#include "game/objects/general/waterfall.h"

#include "game/effects.h"
#include "game/items.h"
#include "game/objects/common.h"
#include "game/random.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/utils.h>

#define WATERFALL_RANGE (WALL_L * 10) // = 10240

void Waterfall_Setup(OBJECT *obj)
{
    obj->control = Waterfall_Control;
    obj->draw_routine = Object_DrawDummyItem;
    obj->save_flags = 1;
}

void Waterfall_Control(int16_t item_num)
{
    ITEM *item = &g_Items[item_num];
    if ((item->flags & IF_CODE_BITS) != IF_CODE_BITS) {
        return;
    }

    int32_t x = item->pos.x - g_LaraItem->pos.x;
    int32_t y = item->pos.y - g_LaraItem->pos.y;
    int32_t z = item->pos.z - g_LaraItem->pos.z;

    if (ABS(x) <= WATERFALL_RANGE && ABS(z) <= WATERFALL_RANGE
        && ABS(y) <= WATERFALL_RANGE) {
        int16_t fx_num = Effect_Create(item->room_num);
        if (fx_num != NO_ITEM) {
            FX *fx = &g_Effects[fx_num];
            fx->pos.x = item->pos.x
                + ((Random_GetDraw() - 0x4000) << WALL_SHIFT) / 0x7FFF;
            fx->pos.z = item->pos.z
                + ((Random_GetDraw() - 0x4000) << WALL_SHIFT) / 0x7FFF;
            fx->pos.y = item->pos.y;
            fx->speed = 0;
            fx->frame_num = 0;
            fx->object_id = O_SPLASH_1;
        }
    }
}
