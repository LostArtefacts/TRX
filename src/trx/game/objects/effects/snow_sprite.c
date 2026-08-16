#include <trx/core/math.h>
#include <trx/game/const.h>
#include <trx/game/effects.h>
#include <trx/game/objects.h>

static void M_Control(const int16_t effect_num)
{
    EFFECT *const effect = Effect_Get(effect_num);
    const OBJECT *const obj = Object_Get(effect->object_id);

    effect->frame_num--;
    if (effect->frame_num <= obj->mesh_count) {
        Effect_Destroy(effect_num);
        return;
    }

    effect->pos = XYZ_32_OffsetYaw(effect->pos, effect->rot.y, effect->speed);
    if (effect->fall_speed != 0) {
        effect->pos.y += effect->fall_speed;
        effect->fall_speed += GRAVITY;
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->effect_control_func = M_Control;
}

REGISTER_OBJECT(O_SNOW_SPRITE, M_Setup)
