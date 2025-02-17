#include "game/objects/effects/blood.h"

#include "game/effects.h"
#include "game/objects/common.h"
#include "global/vars.h"

#include <libtrx/game/math.h>

void Blood_Control(const int16_t effect_num)
{
    EFFECT *const effect = Effect_Get(effect_num);
    const OBJECT *const obj = Object_Get(effect->object_id);
    effect->pos.x += (effect->speed * Math_Sin(effect->rot.y)) >> W2V_SHIFT;
    effect->pos.z += (effect->speed * Math_Cos(effect->rot.y)) >> W2V_SHIFT;
    effect->counter++;
    if (effect->counter == 4) {
        effect->frame_num--;
        effect->counter = 0;
        if (effect->frame_num <= obj->mesh_count) {
            Effect_Kill(effect_num);
        }
    }
}

void Blood_Setup(void)
{
    OBJECT *const obj = Object_Get(O_BLOOD);
    obj->control_func = Blood_Control;
    obj->semi_transparent = 1;
}
