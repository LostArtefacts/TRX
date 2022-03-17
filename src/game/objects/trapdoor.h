#pragma once

#include "global/types.h"

#include <stdint.h>

void TrapDoor_Setup(OBJECT_INFO *obj);
void TrapDoor_Control(int16_t item_num);
void TrapDoor_Floor(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height);
void TrapDoor_Ceiling(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height);
