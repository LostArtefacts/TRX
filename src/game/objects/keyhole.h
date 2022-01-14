#pragma once

#include "global/types.h"

#include <stdint.h>

extern PHD_VECTOR g_KeyHolePosition;
extern int16_t g_KeyHoleBounds[12];

extern int32_t g_PickUpX;
extern int32_t g_PickUpY;
extern int32_t g_PickUpZ;

void SetupKeyHole(OBJECT_INFO *obj);
void KeyHoleCollision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
int32_t KeyTrigger(int16_t item_num);
