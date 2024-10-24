#pragma once

#include "global/types.h"

typedef enum {
    DOOR_STATE_CLOSED = 0,
    DOOR_STATE_OPEN = 1,
} DOOR_STATE;

void __cdecl Door_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
void __cdecl Object_Collision_Trap(
    int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
