#pragma once

#include "../../collision.h"

typedef enum {
    DOOR_STATE_CLOSED = 0,
    DOOR_STATE_OPEN = 1,
} DOOR_STATE;

extern void Door_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
