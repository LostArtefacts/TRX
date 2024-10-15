#include "game/objects/creatures/spider.h"

#include "game/creature.h"
#include "game/objects/common.h"
#include "global/const.h"
#include "global/funcs.h"
#include "global/vars.h"

#define SPIDER_HITPOINTS 5
#define SPIDER_RADIUS (WALL_L / 10) // = 102

void Spider_Setup(void)
{
    OBJECT *const obj = &g_Objects[O_SPIDER];
    if (!obj->loaded) {
        return;
    }

    obj->control = Spider_Control;
    obj->collision = Creature_Collision;

    obj->hit_points = SPIDER_HITPOINTS;
    obj->radius = SPIDER_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;

    obj->intelligent = 1;
    obj->save_anim = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;
}
