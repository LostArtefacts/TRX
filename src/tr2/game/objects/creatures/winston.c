#include "game/objects/creatures/winston.h"

#include "game/creature.h"
#include "game/objects/common.h"
#include "global/const.h"
#include "global/funcs.h"
#include "global/vars.h"

#define WINSTON_RADIUS (WALL_L / 10) // = 102

void Winston_Setup(void)
{
    OBJECT *const obj = &g_Objects[O_WINSTON];
    if (!obj->loaded) {
        return;
    }

    obj->control = Winston_Control;
    obj->collision = Object_Collision;

    obj->hit_points = DONT_TARGET;
    obj->radius = WINSTON_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 4;

    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_anim = 1;
}
