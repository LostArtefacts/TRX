#include "game/objects/vehicles/skidoo_fast.h"

#include "decomp/skidoo.h"

void SkidooFast_Setup(void)
{
    OBJECT *const obj = Object_Get(O_SKIDOO_FAST);
    obj->initialise_func = Skidoo_Initialise;
    obj->draw_func = Skidoo_Draw;
    obj->collision_func = Skidoo_Collision;
    obj->save_position = 1;
    obj->save_flags = 1;
    obj->save_anim = 1;
}
