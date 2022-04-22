#include "game/objects/effects/waterfall.h"

#include "game/draw.h"
#include "game/items.h"
#include "game/random.h"
#include "global/vars.h"

#define WATERFALL_RANGE (WALL_L * 10) // = 10240

void Waterfall_Setup(OBJECT_INFO *obj)
{
    g_Objects[O_WATERFALL].control = Waterfall_Control;
    g_Objects[O_WATERFALL].draw_routine = DrawDummyItem;
}

void Waterfall_Control(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    if ((item->flags & IF_CODE_BITS) != IF_CODE_BITS) {
        return;
    }

    int32_t x = item->pos.x - g_LaraItem->pos.x;
    int32_t y = item->pos.y - g_LaraItem->pos.y;
    int32_t z = item->pos.z - g_LaraItem->pos.z;

    if (ABS(x) <= WATERFALL_RANGE && ABS(z) <= WATERFALL_RANGE
        && ABS(y) <= WATERFALL_RANGE) {
        int16_t fx_num = Effect_Create(item->room_number);
        if (fx_num != NO_ITEM) {
            FX_INFO *fx = &g_Effects[fx_num];
            fx->pos.x = item->pos.x
                + ((Random_GetDraw() - 0x4000) << WALL_SHIFT) / 0x7FFF;
            fx->pos.z = item->pos.z
                + ((Random_GetDraw() - 0x4000) << WALL_SHIFT) / 0x7FFF;
            fx->pos.y = item->pos.y;
            fx->speed = 0;
            fx->frame_number = 0;
            fx->object_number = O_SPLASH1;
        }
    }
}
