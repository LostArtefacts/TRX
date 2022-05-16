#include "game/objects/effects/earthquake.h"

#include "game/items.h"
#include "game/objects/common.h"
#include "game/random.h"
#include "game/sound.h"
#include "global/vars.h"

void Earthquake_Setup(OBJECT_INFO *obj)
{
    obj->control = Earthquake_Control;
    obj->draw_routine = Object_DrawDummyItem;
    obj->save_flags = 1;
}

void Earthquake_Control(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    if (Item_IsTriggerActive(item)) {
        if (Random_GetDraw() < 0x100) {
            g_Camera.bounce = -150;
            Sound_Effect(SFX_ROLLING_BALL, NULL, SPM_NORMAL);
        } else if (Random_GetControl() < 0x400) {
            g_Camera.bounce = 50;
            Sound_Effect(SFX_T_REX_FOOTSTOMP, NULL, SPM_NORMAL);
        }
    }
}
