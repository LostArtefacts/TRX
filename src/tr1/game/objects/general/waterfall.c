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
    obj->control_func = Waterfall_Control;
    obj->draw_func = Object_DrawDummyItem;
    obj->save_flags = 1;
}

void Waterfall_Control(int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if ((item->flags & IF_CODE_BITS) != IF_CODE_BITS) {
        return;
    }

    int32_t x = item->pos.x - g_LaraItem->pos.x;
    int32_t y = item->pos.y - g_LaraItem->pos.y;
    int32_t z = item->pos.z - g_LaraItem->pos.z;

    if (ABS(x) <= WATERFALL_RANGE && ABS(z) <= WATERFALL_RANGE
        && ABS(y) <= WATERFALL_RANGE) {
        int16_t effect_num = Effect_Create(item->room_num);
        if (effect_num != NO_EFFECT) {
            EFFECT *effect = Effect_Get(effect_num);
            effect->pos.x = item->pos.x
                + ((Random_GetDraw() - 0x4000) << WALL_SHIFT) / 0x7FFF;
            effect->pos.z = item->pos.z
                + ((Random_GetDraw() - 0x4000) << WALL_SHIFT) / 0x7FFF;
            effect->pos.y = item->pos.y;
            effect->speed = 0;
            effect->frame_num = 0;
            effect->object_id = O_SPLASH_1;
        }
    }
}
