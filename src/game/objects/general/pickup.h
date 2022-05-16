#pragma once

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

void Pickup_Setup(OBJECT_INFO *obj);
void Pickup_Collision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
void Pickup_CollisionControlled(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
bool Pickup_Trigger(int16_t item_num);
