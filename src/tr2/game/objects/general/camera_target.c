#include "game/objects/general/camera_target.h"

#include "game/objects/common.h"

void CameraTarget_Setup(void)
{
    OBJECT *const obj = Object_Get(O_CAMERA_TARGET);
    obj->draw_func = Object_DrawDummyItem;
}
