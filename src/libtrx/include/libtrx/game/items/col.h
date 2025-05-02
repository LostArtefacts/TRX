#pragma once

#include "./types.h"

int16_t Item_GetHeight(const ITEM *item);
int32_t Item_GetDistance(const ITEM *item, const XYZ_32 *target);
void Item_Translate(ITEM *item, int32_t x, int32_t y, int32_t z);
bool Item_Test3DRange(int32_t x, int32_t y, int32_t z, int32_t range);

bool Item_TestBoundsCollide(
    const ITEM *src_item, const ITEM *dst_item, int32_t radius);
const BOUNDS_16 *Item_GetBoundsAccurate(const ITEM *item);
