#pragma once

#include "global/types.h"

typedef struct GAME_OBJECT_PAIR {
    const GAME_OBJECT_ID key_id;
    const GAME_OBJECT_ID value_id;
} GAME_OBJECT_PAIR;

OBJECT *Object_Get(GAME_OBJECT_ID object_id);
GAME_OBJECT_ID Object_GetCognate(
    GAME_OBJECT_ID key_id, const GAME_OBJECT_PAIR *test_map);
GAME_OBJECT_ID Object_GetCognateInverse(
    GAME_OBJECT_ID value_id, const GAME_OBJECT_PAIR *test_map);
bool Object_IsObjectType(
    GAME_OBJECT_ID object_id, const GAME_OBJECT_ID *test_arr);

void __cdecl Object_Collision(
    int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
