#pragma once

#include "global/types.h"

extern int16_t g_PickupBounds[];

void Pickup_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
bool Pickup_Trigger(int16_t item_num);
