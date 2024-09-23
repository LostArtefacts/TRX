#include "game/objects/general/camera_target.h"

#include "game/objects/common.h"

void CameraTarget_Setup(OBJECT *obj)
{
    obj->draw_routine = Object_DrawDummyItem;
}
