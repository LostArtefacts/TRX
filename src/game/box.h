#pragma once

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

bool Box_SearchLOT(LOT_INFO *LOT, int32_t expansion);
bool Box_UpdateLOT(LOT_INFO *LOT, int32_t expansion);
void Box_TargetBox(LOT_INFO *LOT, int16_t box_number);
bool Box_StalkBox(ITEM_INFO *item, int16_t box_number);
bool Box_EscapeBox(ITEM_INFO *item, int16_t box_number);
bool Box_ValidBox(ITEM_INFO *item, int16_t zone_number, int16_t box_number);
TARGET_TYPE Box_CalculateTarget(
    PHD_VECTOR *target, ITEM_INFO *item, LOT_INFO *LOT);
int32_t BadFloor(
    int32_t x, int32_t y, int32_t z, int16_t box_height, int16_t next_height,
    int16_t room_number, LOT_INFO *LOT);
