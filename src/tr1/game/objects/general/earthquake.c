#include "game/objects/general/earthquake.h"

#include "game/camera.h"
#include "game/items.h"
#include "game/objects/common.h"
#include "game/random.h"
#include "game/sound.h"
#include "global/vars.h"

void Earthquake_Setup(OBJECT *obj)
{
    obj->control_func = Earthquake_Control;
    obj->draw_func = Object_DrawDummyItem;
    obj->save_flags = 1;
}

void Earthquake_Control(int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (Item_IsTriggerActive(item)) {
        if (Random_GetDraw() < 0x100) {
            g_Camera.bounce = -150;
            Sound_Effect(SFX_ROLLING_BALL, nullptr, SPM_NORMAL);
        } else if (Random_GetControl() < 0x400) {
            g_Camera.bounce = 50;
            Sound_Effect(SFX_T_REX_FOOTSTOMP, nullptr, SPM_NORMAL);
        }
    }
}
