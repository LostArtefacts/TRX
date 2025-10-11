#include "config.h"
#include "game/effects.h"
#include "game/objects.h"
#include "game/output.h"
#include "game/random.h"

static void M_Control(const int16_t effect_num)
{
    EFFECT *const effect = Effect_Get(effect_num);

    effect->counter--;
    if (effect->counter == 0) {
        Effect_Kill(effect_num);
        return;
    }

    effect->rot.z = Random_GetControl();
    if (g_Config.visuals.enable_gun_lighting) {
        Output_AddDynamicLight(effect->pos, 12, 11);
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
}

REGISTER_OBJECT(O_GUN_FLASH, M_Setup)
