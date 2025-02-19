#include "game/effects.h"
#include "game/objects/common.h"
#include "game/output.h"
#include "global/vars.h"

#include <libtrx/config.h>

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
    effect->counter++;
    if (effect->counter == 2) {
        effect->frame_num--;
        effect->counter = 0;
        if (g_Config.visuals.enable_gun_lighting
            && effect->frame_num > obj->mesh_count) {
            Output_AddDynamicLight(effect->pos, 13, 11);
        } else if (effect->frame_num <= obj->mesh_count) {
            Effect_Kill(effect_num);
        }
    } else if (g_Config.visuals.enable_gun_lighting) {
        Output_AddDynamicLight(effect->pos, 12, 10);
    }
}

REGISTER_OBJECT(O_EXPLOSION, M_Setup)
