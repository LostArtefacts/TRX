#include "game/objects/common.h"
#include "game/objects/traps/propeller.h"

static void M_Setup(OBJECT *obj);

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = Propeller_Control;
    obj->collision_func = Object_Collision;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

REGISTER_OBJECT(O_POWER_SAW, M_Setup)
