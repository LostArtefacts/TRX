#include "game/objects/creatures/monk.h"

#include "game/creature.h"
#include "game/objects/common.h"
#include "global/const.h"
#include "global/funcs.h"
#include "global/vars.h"

#define MONK_HITPOINTS 30
#define MONK_RADIUS (WALL_L / 10) // = 102

void Monk1_Setup(void)
{
    OBJECT *const obj = &g_Objects[O_MONK_1];
    if (!obj->loaded) {
        return;
    }

    obj->control = Monk_Control;
    obj->collision = Creature_Collision;

    obj->hit_points = MONK_HITPOINTS;
    obj->radius = MONK_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = 0;

    obj->intelligent = 1;
    obj->save_anim = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;

    g_AnimBones[obj->bone_idx + 24] |= BF_ROT_Y;
}

void Monk2_Setup(void)
{
    OBJECT *const obj = &g_Objects[O_MONK_2];
    if (!obj->loaded) {
        return;
    }

    obj->control = Monk_Control;
    obj->collision = Creature_Collision;

    obj->hit_points = MONK_HITPOINTS;
    obj->radius = MONK_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;

    obj->intelligent = 1;
    obj->save_anim = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;
}
