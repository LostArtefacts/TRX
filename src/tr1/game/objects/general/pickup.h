#pragma once

#include "global/types.h"

void Pickup_Setup(OBJECT *obj);
const OBJECT_BOUNDS *Pickup_Bounds(void);
void Pickup_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
void Pickup_CollisionControlled(
    int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
bool Pickup_Trigger(int16_t item_num);
