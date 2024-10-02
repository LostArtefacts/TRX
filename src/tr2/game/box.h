#pragma once

#include "global/types.h"

#define BOX_BLOCKED 0x4000
#define BOX_BLOCKED_SEARCH 0x8000
#define BOX_BLOCKABLE 0x8000
#define BOX_ZONE(num) (((num) / STEP_L) - 1)

int32_t __cdecl Box_SearchLOT(LOT_INFO *lot, int32_t expansion);
int32_t __cdecl Box_UpdateLOT(LOT_INFO *lot, int32_t expansion);
void __cdecl Box_TargetBox(LOT_INFO *lot, int16_t box_num);
int32_t __cdecl Box_StalkBox(
    const ITEM *item, const ITEM *enemy, int16_t box_num);
int32_t __cdecl Box_EscapeBox(
    const ITEM *item, const ITEM *enemy, int16_t box_num);
int32_t __cdecl Box_ValidBox(
    const ITEM *item, int16_t zone_num, int16_t box_num);
TARGET_TYPE __cdecl Box_CalculateTarget(
    XYZ_32 *target, const ITEM *item, LOT_INFO *lot);
int32_t __cdecl Box_BadFloor(
    int32_t x, int32_t y, int32_t z, int32_t box_height, int32_t next_height,
    int16_t room_num, const LOT_INFO *lot);
