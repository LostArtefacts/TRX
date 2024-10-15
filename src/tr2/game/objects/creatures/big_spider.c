#include "game/objects/creatures/big_spider.h"

#include "game/creature.h"
#include "game/objects/common.h"
#include "global/funcs.h"

#define BIG_SPIDER_HITPOINTS 40
#define BIG_SPIDER_RADIUS (WALL_L / 3) // = 341

void BigSpider_Setup(void)
{
    OBJECT *const obj = Object_GetObject(O_BIG_SPIDER);
    if (!obj->loaded) {
        return;
    }

    obj->control = BigSpider_Control;
    obj->collision = Creature_Collision;

    obj->hit_points = BIG_SPIDER_HITPOINTS;
    obj->radius = BIG_SPIDER_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;

    obj->intelligent = 1;
    obj->save_anim = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;
}
