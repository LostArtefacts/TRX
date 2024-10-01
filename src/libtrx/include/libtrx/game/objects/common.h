#pragma once

#include "../anims.h"
#include "../collision.h"
#include "../items.h"
#include "../math.h"
#include "ids.h"
#include "types.h"

OBJECT *Object_GetObject(GAME_OBJECT_ID object_id);

bool Object_IsObjectType(
    GAME_OBJECT_ID object_id, const GAME_OBJECT_ID *test_arr);
