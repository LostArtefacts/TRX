#include "game/objects/misc.h"

#include "game/objects.h"
#include "game/objects/cog.h"
#include "game/objects/pickup.h"

void CameraTarget_Setup(OBJECT_INFO *obj)
{
    obj->draw_routine = Object_DrawDummyItem;
}

void MovingBar_Setup(OBJECT_INFO *obj)
{
    obj->control = Cog_Control;
    obj->collision = Object_Collision;
    obj->save_flags = 1;
    obj->save_anim = 1;
    obj->save_position = 1;
}

void LeadBar_Setup(OBJECT_INFO *obj)
{
    obj->draw_routine = Object_DrawPickupItem;
    obj->collision = Pickup_Collision;
    obj->save_flags = 1;
}
