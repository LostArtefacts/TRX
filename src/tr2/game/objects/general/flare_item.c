#include "game/objects/general/flare_item.h"

#include "decomp/flares.h"
#include "game/objects/general/pickup.h"

void FlareItem_Setup(void)
{
    OBJECT *const obj = Object_Get(O_FLARE_ITEM);
    obj->collision_func = Pickup_Collision;
    obj->control_func = Flare_Control;
    obj->draw_func = Flare_DrawInAir;
    obj->save_position = 1;
    obj->save_flags = 1;
}
