#pragma once

#include "global/types.h"

#include <stdint.h>

void SetupPickupObject(OBJECT_INFO *obj);
void PickUpCollision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
void PickUpCollisionAnim(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
