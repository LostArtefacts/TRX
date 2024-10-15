#include "game/objects/creatures/mouse.h"

#include "game/creature.h"
#include "game/objects/common.h"
#include "global/const.h"
#include "global/funcs.h"
#include "global/vars.h"

#define MOUSE_HITPOINTS 4
#define MOUSE_RADIUS (WALL_L / 10) // = 102

void Mouse_Setup(void)
{
    OBJECT *const obj = &g_Objects[O_MOUSE];
    if (!obj->loaded) {
        return;
    }

    obj->control = Mouse_Control;
    obj->collision = Creature_Collision;

    obj->hit_points = MOUSE_HITPOINTS;
    obj->radius = MOUSE_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = 50;

    obj->intelligent = 1;
    obj->save_anim = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;
}
