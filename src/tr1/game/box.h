#pragma once

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

bool Box_SearchLOT(LOT_INFO *lot, int32_t expansion);
bool Box_UpdateLOT(LOT_INFO *lot, int32_t expansion);
void Box_TargetBox(LOT_INFO *lot, int16_t box_num);
bool Box_StalkBox(ITEM *item, int16_t box_num);
bool Box_EscapeBox(ITEM *item, int16_t box_num);
bool Box_ValidBox(ITEM *item, int16_t zone_num, int16_t box_num);
TARGET_TYPE Box_CalculateTarget(XYZ_32 *target, ITEM *item, LOT_INFO *lot);
bool Box_BadFloor(
    int32_t x, int32_t y, int32_t z, int16_t box_height, int16_t next_height,
    int16_t room_num, LOT_INFO *lot);
