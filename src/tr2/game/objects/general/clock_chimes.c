#include "game/objects/general/clock_chimes.h"

#include "game/items.h"
#include "game/objects/common.h"
#include "game/sound.h"

#include <libtrx/game/lara/common.h>

void DoChimeSound(const ITEM *const item)
{
    const ITEM *const lara_item = Lara_GetItem();
    XYZ_32 pos = lara_item->pos;
    pos.x += (item->pos.x - lara_item->pos.x) >> 6;
    pos.y += (item->pos.y - lara_item->pos.y) >> 6;
    pos.z += (item->pos.z - lara_item->pos.z) >> 6;
    Sound_Effect(SFX_DOOR_CHIME, &pos, SPM_NORMAL);
}

void ClockChimes_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (item->timer == 0) {
        return;
    }
    if (item->timer % 60 == 59) {
        DoChimeSound(item);
    }
    item->timer--;
    if (item->timer == 0) {
        DoChimeSound(item);
        item->timer = -1;
        Item_RemoveActive(item_num);
        item->status = IS_INACTIVE;
        item->flags &= ~IF_CODE_BITS;
    }
}

void ClockChimes_Setup(void)
{
    OBJECT *const obj = Object_Get(O_CLOCK_CHIMES);
    obj->control_func = ClockChimes_Control;
    obj->draw_func = Object_DrawDummyItem;
    obj->save_flags = 1;
}
