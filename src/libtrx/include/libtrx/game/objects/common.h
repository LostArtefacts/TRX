#pragma once

#include "../anims.h"
#include "../collision.h"
#include "../items.h"
#include "../math.h"
#include "ids.h"
#include "types.h"

typedef struct {
    const GAME_OBJECT_ID key_id;
    const GAME_OBJECT_ID value_id;
} GAME_OBJECT_PAIR;

extern OBJECT *Object_GetObject(GAME_OBJECT_ID object_id);

bool Object_IsObjectType(
    GAME_OBJECT_ID object_id, const GAME_OBJECT_ID *test_arr);

GAME_OBJECT_ID Object_GetCognate(
    GAME_OBJECT_ID key_id, const GAME_OBJECT_PAIR *test_map);

GAME_OBJECT_ID Object_GetCognateInverse(
    GAME_OBJECT_ID value_id, const GAME_OBJECT_PAIR *test_map);
