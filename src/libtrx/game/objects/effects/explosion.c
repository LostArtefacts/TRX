#include "config.h"
#include "game/effects.h"
#include "game/objects.h"
#include "game/output.h"
#include "version.h"

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

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->semi_transparent = g_TRVersion == 2;
}

REGISTER_OBJECT(O_EXPLOSION_1, M_Setup)
