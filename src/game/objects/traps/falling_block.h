#pragma once

#include "global/types.h"

void FallingBlock_Setup(OBJECT *obj);
void FallingBlock_Control(int16_t item_num);
int16_t FallingBlock_GetFloorHeight(
    const ITEM *item, int32_t x, int32_t y, int32_t z, int16_t height);
int16_t FallingBlock_GetCeilingHeight(
    const ITEM *item, int32_t x, int32_t y, int32_t z, int16_t height);
