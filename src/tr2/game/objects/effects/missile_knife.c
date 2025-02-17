#include "game/objects/effects/missile_knife.h"

#include "game/objects/common.h"
#include "game/objects/effects/missile_common.h"

void MissileSpawn_Knife_Setup(void)
{
    OBJECT *const obj = Object_Get(O_MISSILE_KNIFE);
    obj->control_func = Missile_Control;
    obj->save_position = 1;
}
