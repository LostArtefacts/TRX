#include "game/collide.h"
#include "game/draw.h"
#include "game/objects/cog.h"
#include "game/objects/misc.h"
#include "game/objects/pickup.h"

void SetupCameraTarget(OBJECT_INFO *obj)
{
    obj->draw_routine = DrawDummyItem;
}

void SetupMovingBar(OBJECT_INFO *obj)
{
    obj->control = CogControl;
    obj->collision = ObjectCollision;
    obj->save_flags = 1;
    obj->save_anim = 1;
    obj->save_position = 1;
}

void SetupLeadBar(OBJECT_INFO *obj)
{
    obj->draw_routine = DrawSpriteItem;
    obj->collision = PickUpCollision;
    obj->save_flags = 1;
}
