#include "game/camera.h"
#include "game/items.h"
#include "game/objects/common.h"
#include "game/random.h"
#include "game/sound.h"

static void M_Setup(OBJECT *obj);
static void M_Control(int16_t effect_num);

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->draw_func = Object_DrawDummyItem;
    obj->save_flags = 1;
}

static void M_Control(const int16_t item_num)
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

REGISTER_OBJECT(O_EARTHQUAKE, M_Setup)
