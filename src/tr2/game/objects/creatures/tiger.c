#include "game/objects/creatures/tiger.h"

#include "game/creature.h"
#include "game/objects/common.h"
#include "global/const.h"
#include "global/funcs.h"
#include "global/vars.h"

#define TIGER_HITPOINTS 20
#define TIGER_RADIUS (WALL_L / 3) // = 341

void Tiger_Setup(void)
{
    OBJECT *const obj = &g_Objects[O_TIGER];
    if (!obj->loaded) {
        return;
    }

    obj->control = Tiger_Control;
    obj->collision = Creature_Collision;

    obj->hit_points = TIGER_HITPOINTS;
    obj->radius = TIGER_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = 200;

    obj->intelligent = 1;
    obj->save_anim = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;
}
