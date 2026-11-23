#include <trx/game/objects.h>

static void M_Setup(OBJECT *const obj)
{
    obj->collision_func = Object_Collision;
}

REGISTER_OBJECT(O_HORUS_STATUE, M_Setup)
