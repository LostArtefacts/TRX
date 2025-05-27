#include "decomp/flares.h"
#include "game/objects/general/pickup.h"

static void M_Setup(OBJECT *obj);

static void M_Setup(OBJECT *const obj)
{
    obj->collision_func = Pickup_Collision;
    obj->bounds_func = Pickup_Bounds;
    obj->control_func = Flare_Control;
    obj->draw_func = Flare_DrawInAir;
    obj->save_position = true;
    obj->save_flags = true;
}

REGISTER_OBJECT(O_FLARE_ITEM, M_Setup)
