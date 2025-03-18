#include "game/effects.h"
#include "game/objects/common.h"

#include <libtrx/game/math.h>

static void M_Setup(OBJECT *obj);
static void M_Control(int16_t effect_num);

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
}

static void M_Control(const int16_t effect_num)
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

REGISTER_OBJECT(O_BLOOD_1, M_Setup)
