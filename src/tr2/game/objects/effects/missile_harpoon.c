#include "game/objects/common.h"
#include "game/objects/effects/missile_common.h"

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = Missile_Control;
    obj->save_position = true;
}

REGISTER_OBJECT(O_MISSILE_HARPOON, M_Setup)
