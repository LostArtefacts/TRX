#pragma once

#include "global/types.h"

void TrapDoor_Setup(OBJECT *obj);
void TrapDoor_Control(int16_t item_num);
int16_t TrapDoor_GetFloorHeight(
    const ITEM *item, int32_t x, int32_t y, int32_t z, int16_t height);
int16_t TrapDoor_GetCeilingHeight(
    const ITEM *item, int32_t x, int32_t y, int32_t z, int16_t height);
