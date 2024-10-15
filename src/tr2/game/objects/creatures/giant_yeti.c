#include "game/objects/creatures/giant_yeti.h"

#include "game/creature.h"
#include "game/objects/common.h"
#include "global/const.h"
#include "global/funcs.h"
#include "global/vars.h"

#define GIANT_YETI_HITPOINTS 200
#define GIANT_YETI_RADIUS (WALL_L / 3) // = 341

void GiantYeti_Setup(void)
{
    OBJECT *const obj = &g_Objects[O_GIANT_YETI];
    if (!obj->loaded) {
        return;
    }

    obj->control = GiantYeti_Control;
    obj->collision = Creature_Collision;

    obj->hit_points = GIANT_YETI_HITPOINTS;
    obj->radius = GIANT_YETI_RADIUS;

    obj->intelligent = 1;
    obj->save_anim = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;
}
