#include "game/objects/common.h"
#include "game/objects/effects/missile_common.h"

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = Missile_Control;
    obj->semi_transparent = true;
}

REGISTER_OBJECT(O_MISSILE_FLAME, M_Setup)
