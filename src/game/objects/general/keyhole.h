#pragma once

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

extern XYZ_32 g_KeyHolePosition;
extern int16_t g_KeyHoleBounds[12];

extern int32_t g_PickUpX;
extern int32_t g_PickUpY;
extern int32_t g_PickUpZ;

void KeyHole_Setup(OBJECT_INFO *obj);
void KeyHole_Collision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
bool KeyHole_Trigger(int16_t item_num);
