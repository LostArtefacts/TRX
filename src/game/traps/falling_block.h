#pragma once

#include "global/types.h"

#include <stdint.h>

void SetupFallingBlock(OBJECT_INFO *obj);
void FallingBlockControl(int16_t item_num);
void FallingBlockFloor(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int32_t *height);
void FallingBlockCeiling(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int32_t *height);
