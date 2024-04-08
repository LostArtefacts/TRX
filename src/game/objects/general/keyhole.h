#pragma once

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

extern XYZ_32 g_KeyHolePosition;

void KeyHole_Setup(OBJECT_INFO *obj);
void KeyHole_Collision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
bool KeyHole_Trigger(int16_t item_num);
