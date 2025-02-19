#include "decomp/flares.h"
#include "game/objects/general/pickup.h"

static void M_Setup(OBJECT *obj);

static void M_Setup(OBJECT *const obj)
{
    obj->collision_func = Pickup_Collision;
    obj->control_func = Flare_Control;
    obj->draw_func = Flare_DrawInAir;
    obj->save_position = 1;
    obj->save_flags = 1;
}

REGISTER_OBJECT(O_FLARE_ITEM, M_Setup)
