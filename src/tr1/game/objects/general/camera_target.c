#include "game/objects/common.h"

static void M_Setup(OBJECT *obj);

static void M_Setup(OBJECT *const obj)
{
    obj->draw_func = Object_DrawDummyItem;
}

REGISTER_OBJECT(O_CAMERA_TARGET, M_Setup)
