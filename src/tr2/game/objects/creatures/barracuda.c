#include "game/objects/creatures/barracuda.h"

#include "game/creature.h"
#include "game/objects/common.h"
#include "global/funcs.h"
#include "global/vars.h"

#define BARRACUDA_HITPOINTS 12
#define BARRACUDA_RADIUS (WALL_L / 5) // = 204

void Barracuda_Setup(void)
{
    OBJECT *const obj = Object_GetObject(O_BARRACUDA);
    if (!obj->loaded) {
        return;
    }

    obj->control = Baracudda_Control;
    obj->collision = Creature_Collision;

    obj->hit_points = BARRACUDA_HITPOINTS;
    obj->radius = BARRACUDA_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = 200;

    obj->intelligent = 1;
    obj->water_creature = 1;
    obj->save_anim = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;

    g_AnimBones[obj->bone_idx + 24] |= BF_ROT_Y;
}
