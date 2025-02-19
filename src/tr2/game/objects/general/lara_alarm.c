#include "game/objects/common.h"
#include "game/sound.h"

static void M_Setup(OBJECT *obj);
static void M_Control(int16_t item_num);

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->draw_func = Object_DrawDummyItem;
    obj->save_flags = 1;
}

static void M_Control(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);
    if ((item->flags & IF_CODE_BITS) == IF_CODE_BITS) {
        Sound_Effect(SFX_BURGLAR_ALARM, &item->pos, SPM_NORMAL);
    }
}

REGISTER_OBJECT(O_LARA_ALARM, M_Setup)
