#include "game/objects/traps/power_saw.h"

#include "game/objects/common.h"
#include "game/objects/traps/propeller.h"

void PowerSaw_Setup(void)
{
    OBJECT *const obj = Object_Get(O_POWER_SAW);
    obj->control_func = Propeller_Control;
    obj->collision_func = Object_Collision;
    obj->save_flags = 1;
    obj->save_anim = 1;
}
