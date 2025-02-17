#include "game/objects/effects/missile_flame.h"

#include "game/objects/common.h"
#include "game/objects/effects/missile_common.h"

void MissileFlame_Setup(void)
{
    OBJECT *const obj = Object_Get(O_MISSILE_FLAME);
    obj->control_func = Missile_Control;
    obj->semi_transparent = 1;
}
