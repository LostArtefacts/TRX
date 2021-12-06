#ifndef T1M_GAME_OBJECTS_PICKUP_H
#define T1M_GAME_OBJECTS_PICKUP_H

#include "global/types.h"

#include <stdint.h>

extern PHD_VECTOR g_PickUpPosition;
extern PHD_VECTOR g_PickUpPositionUW;
extern int16_t g_PickUpBounds[12];
extern int16_t g_PickUpBoundsUW[12];

void SetupPickupObject(OBJECT_INFO *obj);
void PickUpCollision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);

#endif
