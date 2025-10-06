#include "game/objects/common.h"

#include <libtrx/game/camera.h>
#include <libtrx/game/random.h>
#include <libtrx/game/sound.h>

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (Item_IsTriggerActive(item)) {
        if (Random_GetDraw() < 0x100) {
            g_Camera.bounce = -150;
            Sound_Effect(SFX_ROLLING_BALL, nullptr, SPM_NORMAL);
        } else if (Random_GetControl() < 0x400) {
            g_Camera.bounce = 50;
            Sound_Effect(SFX_T_REX_STOMP, nullptr, SPM_NORMAL);
        }
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->draw_func = nullptr;
    obj->save_flags = true;
}

REGISTER_OBJECT(O_EARTHQUAKE, M_Setup)
