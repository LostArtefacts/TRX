#include "game/objects/vehicles/skidoo_armed.h"

#include "game/objects/creatures/skidoo_driver.h"
#include "global/funcs.h"
#include "global/vars.h"

#define SKIDOO_ARMED_RADIUS (WALL_L / 3) // = 341

void SkidooArmed_Setup(void)
{
    OBJECT *const obj = &g_Objects[O_SKIDOO_ARMED];
    if (!obj->loaded) {
        return;
    }

    obj->collision = SkidooArmed_Collision;

    obj->hit_points = SKIDOO_DRIVER_HITPOINTS;
    obj->radius = SKIDOO_ARMED_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = 0;

    obj->intelligent = 1;
    obj->save_anim = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;
}
