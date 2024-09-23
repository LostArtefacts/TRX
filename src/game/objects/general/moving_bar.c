#include "game/objects/general/moving_bar.h"

#include "game/objects/common.h"
#include "game/objects/general/cog.h"

void MovingBar_Setup(OBJECT *obj)
{
    obj->control = Cog_Control;
    obj->collision = Object_Collision;
    obj->save_flags = 1;
    obj->save_anim = 1;
    obj->save_position = 1;
}
