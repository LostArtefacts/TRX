#include "game/effects.h"
#include "global/vars.h"

#include <libtrx/game/math.h>

static void M_Setup(OBJECT *obj);
static void M_Control(int16_t effect_num);

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
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

    effect->pos.z += (effect->speed * Math_Cos(effect->rot.y)) >> W2V_SHIFT;
    effect->pos.x += (effect->speed * Math_Sin(effect->rot.y)) >> W2V_SHIFT;
    if (effect->fall_speed != 0) {
        effect->pos.y += effect->fall_speed;
        effect->fall_speed += GRAVITY;
    }
}

REGISTER_OBJECT(O_SNOW_SPRITE, M_Setup)
