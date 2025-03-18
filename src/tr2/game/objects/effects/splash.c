#include "game/effects.h"
#include "game/objects/common.h"
#include "global/vars.h"

#include <libtrx/game/math.h>

static void M_Setup(OBJECT *obj);
static void M_Control(int16_t effect_num);

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->semi_transparent = 1;
}

static void M_Control(const int16_t effect_num)
{
    EFFECT *const effect = Effect_Get(effect_num);
    const OBJECT *const obj = Object_Get(effect->object_id);

    effect->frame_num--;
    if (effect->frame_num <= obj->mesh_count) {
        Effect_Kill(effect_num);
        return;
    }

    effect->pos.x += (effect->speed * Math_Sin(effect->rot.y)) >> W2V_SHIFT;
    effect->pos.z += (effect->speed * Math_Cos(effect->rot.y)) >> W2V_SHIFT;
}

REGISTER_OBJECT(O_SPLASH, M_Setup)
