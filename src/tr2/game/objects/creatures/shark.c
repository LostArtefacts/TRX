#include "game/objects/creatures/shark.h"

#include "game/creature.h"
#include "game/objects/common.h"
#include "global/const.h"
#include "global/funcs.h"
#include "global/vars.h"

#define SHARK_HITPOINTS 30
#define SHARK_RADIUS (WALL_L / 3) // = 341

void Shark_Setup(void)
{
    OBJECT *const obj = &g_Objects[O_SHARK];
    if (!obj->loaded) {
        return;
    }

    obj->control = Shark_Control;
    obj->draw_routine = Object_DrawUnclippedItem;
    obj->collision = Creature_Collision;

    obj->hit_points = SHARK_HITPOINTS;
    obj->radius = SHARK_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = 200;

    obj->intelligent = 1;
    obj->water_creature = 1;
    obj->save_anim = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;
}
