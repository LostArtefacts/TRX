#pragma once

#include "global/types.h"

#include <stdint.h>

void FallingBlock_Setup(OBJECT_INFO *obj);
void FallingBlock_Control(int16_t item_num);
int16_t FallingBlock_GetFloorHeight(
    const ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t height);
int16_t FallingBlock_GetCeilingHeight(
    const ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t height);
