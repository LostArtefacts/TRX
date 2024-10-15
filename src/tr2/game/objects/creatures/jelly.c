#include "game/objects/creatures/jelly.h"

#include "game/creature.h"
#include "game/objects/common.h"
#include "global/const.h"
#include "global/funcs.h"
#include "global/vars.h"

#define JELLY_HITPOINTS 10
#define JELLY_RADIUS (WALL_L / 10) // = 102

void Jelly_Setup(void)
{
    OBJECT *const obj = &g_Objects[O_JELLY];
    if (!obj->loaded) {
        return;
    }

    obj->control = Jelly_Control;
    obj->collision = Creature_Collision;

    obj->hit_points = JELLY_HITPOINTS;
    obj->radius = JELLY_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;

    obj->intelligent = 1;
    obj->save_anim = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;
}
