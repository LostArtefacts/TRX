#include "game/objects/creatures/dog.h"

#include "game/creature.h"
#include "game/objects/common.h"
#include "global/const.h"
#include "global/funcs.h"
#include "global/vars.h"

#define DOG_HITPOINTS 10
#define DOG_RADIUS (WALL_L / 3) // = 341

void Dog_Setup(void)
{
    OBJECT *const obj = Object_GetObject(O_DOG);
    if (!obj->loaded) {
        return;
    }

    obj->control = Dog_Control;
    obj->collision = Creature_Collision;

    obj->hit_points = DOG_HITPOINTS;
    obj->radius = DOG_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = 300;

    obj->save_anim = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;
    obj->intelligent = 1;
}
