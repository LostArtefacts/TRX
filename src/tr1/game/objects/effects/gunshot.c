#include "game/objects/effects/gunshot.h"

#include "game/effects.h"
#include "game/random.h"

#include <libtrx/config.h>
#include <libtrx/game/output.h>

void GunShot_Setup(OBJECT *obj)
{
    obj->control_func = GunShot_Control;
}

void GunShot_Control(int16_t effect_num)
{
    EFFECT *effect = Effect_Get(effect_num);
    effect->counter--;
    if (!effect->counter) {
        Effect_Kill(effect_num);
        return;
    }
    effect->rot.z = Random_GetControl();
    if (g_Config.visuals.enable_gun_lighting) {
        Output_AddDynamicLight(effect->pos, 12, 11);
    }
}
