#include "game/effects.h"
#include "game/objects/common.h"

static void M_Setup(OBJECT *obj);
static void M_Control(int16_t effect_num);

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->draw_func = Object_DrawSpriteItem;
}

static void M_Control(const int16_t effect_num)
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

REGISTER_OBJECT(O_DART_EFFECT, M_Setup)
