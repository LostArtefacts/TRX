#include "game/objects/general/lara_alarm.h"

#include "game/objects/common.h"
#include "game/sound.h"

void LaraAlarm_Control(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);
    if ((item->flags & IF_CODE_BITS) == IF_CODE_BITS) {
        Sound_Effect(SFX_BURGLAR_ALARM, &item->pos, SPM_NORMAL);
    }
}

void LaraAlarm_Setup(void)
{
    OBJECT *const obj = Object_Get(O_LARA_ALARM);
    obj->control_func = LaraAlarm_Control;
    obj->draw_func = Object_DrawDummyItem;
    obj->save_flags = 1;
}
