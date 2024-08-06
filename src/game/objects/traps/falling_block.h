#pragma once

#include "global/types.h"

#include <stdint.h>

void FallingBlock_Setup(OBJECT_INFO *obj);
void FallingBlock_Control(int16_t item_num);
void FallingBlock_AlterFloorHeight(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height);
void FallingBlock_AlterCeilingHeight(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height);
