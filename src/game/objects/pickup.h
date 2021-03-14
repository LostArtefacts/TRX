#ifndef T1M_GAME_OBJECTS_PICKUP_H
#define T1M_GAME_OBJECTS_PICKUP_H

#include "game/types.h"

#include <stdint.h>

extern PHD_VECTOR PickUpPosition;
extern PHD_VECTOR PickUpPositionUW;
extern int16_t PickUpBounds[12];
extern int16_t PickUpBoundsUW[12];

void SetupPickupObject(OBJECT_INFO *obj);
void PickUpCollision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);

#endif
