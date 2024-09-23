#pragma once

#include "global/types.h"

bool Bridge_IsSameSector(int32_t x, int32_t z, const ITEM *item);
int32_t Bridge_GetOffset(const ITEM *item, int32_t x, int32_t y, int32_t z);
void Bridge_FixEmbeddedPosition(int16_t item_num);

void Bridge_Initialise(int16_t item_num);
