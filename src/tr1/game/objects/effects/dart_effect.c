#include "game/objects/effects/dart_effect.h"

#include "game/effects.h"
#include "game/objects/common.h"
#include "global/vars.h"

void DartEffect_Setup(OBJECT *obj)
{
    obj->control_func = DartEffect_Control;
    obj->draw_func = Object_DrawSpriteItem;
}

void DartEffect_Control(int16_t effect_num)
{
    EFFECT *effect = Effect_Get(effect_num);
    effect->counter++;
    if (effect->counter >= 3) {
        effect->counter = 0;
        effect->frame_num--;
        if (effect->frame_num <= Object_Get(effect->object_id)->mesh_count) {
            Effect_Kill(effect_num);
        }
    }
}
