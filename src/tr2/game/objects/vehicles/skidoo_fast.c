#include "decomp/skidoo.h"

static void M_Setup(OBJECT *obj);

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = Skidoo_Initialise;
    obj->draw_func = Skidoo_Draw;
    obj->collision_func = Skidoo_Collision;
    obj->save_position = 1;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

REGISTER_OBJECT(O_SKIDOO_FAST, M_Setup)
