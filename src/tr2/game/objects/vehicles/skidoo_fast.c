#include "decomp/skidoo.h"

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = Skidoo_Initialise;
    obj->draw_func = Skidoo_Draw;
    obj->collision_func = Skidoo_Collision;
    obj->save_position = true;
    obj->save_flags = true;
    obj->save_anim = true;
}

REGISTER_OBJECT(O_SKIDOO_FAST, M_Setup)
