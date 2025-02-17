#include "game/objects/effects/blood.h"

#include "game/effects.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/game/math.h>

void Blood_Setup(OBJECT *obj)
{
    obj->control_func = Blood_Control;
}

void Blood_Control(int16_t effect_num)
{
    EFFECT *effect = Effect_Get(effect_num);
    effect->pos.x += (Math_Sin(effect->rot.y) * effect->speed) >> W2V_SHIFT;
    effect->pos.z += (Math_Cos(effect->rot.y) * effect->speed) >> W2V_SHIFT;
    effect->counter++;
    if (effect->counter == 4) {
        effect->counter = 0;
        effect->frame_num--;
        if (effect->frame_num <= Object_Get(effect->object_id)->mesh_count) {
            Effect_Kill(effect_num);
        }
    }
}
