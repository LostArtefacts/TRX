#pragma once

#include "global/types.h"

typedef enum {
    DOOR_STATE_CLOSED,
    DOOR_STATE_OPEN,
} DOOR_STATE;

void Door_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
