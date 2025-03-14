#pragma once

#include "../items/types.h"
#include "./types.h"

void Box_InitialiseBoxes(int32_t num_boxes);
int16_t *Box_InitialiseOverlaps(int32_t num_overlaps);
int32_t Box_GetCount(void);
BOX_INFO *Box_GetBox(int32_t box_idx);
int16_t Box_GetOverlap(int32_t overlap_idx);
int16_t *Box_GetFlyZone(bool flip_status);
int16_t *Box_GetGroundZone(bool flip_status, int32_t zone_idx);
int16_t *Box_GetLotZone(const LOT_INFO *lot);
bool Box_SearchLOT(LOT_INFO *lot, int32_t expansion);
bool Box_UpdateLOT(LOT_INFO *lot, int32_t expansion);
void Box_TargetBox(LOT_INFO *lot, int16_t box_num);
bool Box_StalkBox(const ITEM *item, const ITEM *enemy, int16_t box_num);
bool Box_EscapeBox(const ITEM *item, const ITEM *enemy, int16_t box_num);
bool Box_ValidBox(const ITEM *item, int16_t zone_num, int16_t box_num);
