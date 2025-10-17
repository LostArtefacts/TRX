#pragma once

#include "../types.h"

bool Pickup_Trigger(int16_t item_num);
const OBJECT_BOUNDS *Pickup_Bounds(void);

extern void Pickup_Collision(
    int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
