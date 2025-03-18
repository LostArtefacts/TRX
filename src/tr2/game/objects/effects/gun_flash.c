#include "game/effects.h"
#include "game/output.h"
#include "game/random.h"
#include "global/vars.h"

#include <libtrx/config.h>

static void M_Setup(OBJECT *obj);
static void M_Control(int16_t effect_num);

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
}

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

REGISTER_OBJECT(O_GUN_FLASH, M_Setup)
