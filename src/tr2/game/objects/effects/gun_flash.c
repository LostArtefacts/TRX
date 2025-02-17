#include "game/objects/effects/gun_flash.h"

#include "game/effects.h"
#include "game/output.h"
#include "game/random.h"
#include "global/vars.h"

#include <libtrx/config.h>

void GunFlash_Control(const int16_t effect_num)
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

void GunFlash_Setup(void)
{
    OBJECT *const obj = Object_Get(O_GUN_FLASH);
    obj->control_func = GunFlash_Control;
}
