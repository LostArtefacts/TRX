#include "game/objects/effects/water_sprite.h"

#include "game/effects.h"
#include "global/vars.h"

#include <libtrx/game/math.h>

void WaterSprite_Control(const int16_t effect_num)
{
    EFFECT *const effect = Effect_Get(effect_num);
    const OBJECT *const obj = Object_Get(effect->object_id);

    effect->counter--;
    if (effect->counter % 4 == 0) {
        effect->frame_num--;
        if (effect->frame_num <= obj->mesh_count) {
            effect->frame_num = 0;
        }
    }

    if (effect->counter == 0 || effect->fall_speed > 0) {
        Effect_Kill(effect_num);
        return;
    }

    effect->pos.x += (effect->speed * Math_Sin(effect->rot.y)) >> W2V_SHIFT;
    effect->pos.z += (effect->speed * Math_Cos(effect->rot.y)) >> W2V_SHIFT;
    if (effect->fall_speed != 0) {
        effect->pos.y += effect->fall_speed;
        effect->fall_speed += GRAVITY;
    }
}

void WaterSprite_Setup(void)
{
    OBJECT *const obj = Object_Get(O_WATER_SPRITE);
    obj->control_func = WaterSprite_Control;
    obj->semi_transparent = 1;
}
