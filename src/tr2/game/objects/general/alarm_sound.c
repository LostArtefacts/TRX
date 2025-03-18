#include "game/output.h"
#include "game/sound.h"

static void M_Setup(OBJECT *obj);
static void M_Control(int16_t item_num);

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->save_flags = 1;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if ((item->flags & IF_CODE_BITS) != IF_CODE_BITS) {
        return;
    }

    Sound_Effect(SFX_PLATFORM_ALARM, &item->pos, SPM_NORMAL);

    int32_t counter = (int32_t)(intptr_t)item->data;
    counter++;
    if (counter > 6) {
        Output_AddDynamicLight(item->pos, 12, 11);
        if (counter > 12) {
            counter = 0;
        }
    }
    item->data = (void *)(intptr_t)counter;
}

REGISTER_OBJECT(O_ALARM_SOUND, M_Setup)
