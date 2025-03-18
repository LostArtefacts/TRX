#include "game/objects/common.h"
#include "game/sound.h"

static void M_Setup(OBJECT *obj);
static void M_Control(int16_t item_num);

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->draw_func = Object_DrawDummyItem;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if ((item->flags & IF_CODE_BITS) == IF_CODE_BITS) {
        Sound_Effect(SFX_DOORBELL, &item->pos, SPM_NORMAL);
        item->flags -= IF_CODE_BITS;
    }
}

REGISTER_OBJECT(O_DING_DONG, M_Setup)
