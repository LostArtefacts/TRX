#pragma once

#include "global/types.h"

void Drawbridge_Setup(void);

int32_t Drawbridge_IsItemOnTop(const ITEM *item, int32_t z, int32_t x);

void Drawbridge_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
