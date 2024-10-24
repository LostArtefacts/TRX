#pragma once

#include "global/types.h"

typedef enum {
    DOOR_STATE_CLOSED = 0,
    DOOR_STATE_OPEN = 1,
} DOOR_STATE;

void __cdecl Door_Shut(DOORPOS_DATA *d);
void __cdecl Door_Open(DOORPOS_DATA *d);

void __cdecl Door_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
