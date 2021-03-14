#ifndef T1M_GAME_OBJECTS_KEYHOLE_H
#define T1M_GAME_OBJECTS_KEYHOLE_H

#include "game/types.h"
#include <stdint.h>

extern PHD_VECTOR KeyHolePosition;
extern int16_t KeyHoleBounds[12];

extern int32_t PickUpX;
extern int32_t PickUpY;
extern int32_t PickUpZ;

void SetupKeyHole(OBJECT_INFO *obj);
void KeyHoleCollision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
int32_t KeyTrigger(int16_t item_num);

#endif
