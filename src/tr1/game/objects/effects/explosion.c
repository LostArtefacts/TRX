#include "game/objects/effects/explosion.h"

#include "game/effects.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/output.h>

void Explosion_Setup(OBJECT *obj)
{
    obj->control_func = Explosion_Control;
}

void Explosion_Control(int16_t effect_num)
{
    EFFECT *effect = Effect_Get(effect_num);
    const OBJECT *const obj = Object_Get(effect->object_id);
    effect->counter++;
    if (effect->counter == 2) {
        effect->counter = 0;
        effect->frame_num--;
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
