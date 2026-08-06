#include <trx/game/effects.h>
#include <trx/game/objects.h>
#include <trx/game/output.h>
#include <trx/game/output/shaders/mesh.h>

static void M_Control(const int16_t effect_num)
{
    EFFECT *const effect = Effect_Get(effect_num);

    effect->counter--;
    if (effect->counter == 0) {
        Effect_Destroy(effect_num);
        return;
    }

    effect->shade += effect->speed;
    effect->frame_num += effect->fall_speed;
}

static void M_Setup(OBJECT *const obj)
{
    obj->effect_control_func = M_Control;
    if (obj->loaded) {
        // The glow is a halo around a light source, so it must always face the
        // camera - the sprite lock modes would let it tilt away from it.
        for (int32_t i = 0; i < -obj->mesh_count; i++) {
            Output_GetSpriteTexture(obj->mesh_idx + i)->flags |=
                VERT_ABS_SPRITE;
        }
    }
}

REGISTER_OBJECT(O_GLOW, M_Setup)
