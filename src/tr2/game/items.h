#pragma once

#include "global/types.h"

void Item_Control(void);
void Item_Initialise(int16_t item_num);
void Item_ClearKilled(void);
void Item_ShiftCol(ITEM *item, COLL_INFO *coll);
void Item_UpdateRoom(ITEM *item, int32_t height);
int32_t Item_TestBoundsCollide(
    const ITEM *src_item, const ITEM *dst_item, int32_t radius);
int32_t Item_TestPosition(
    const int16_t *bounds, const ITEM *src_item, const ITEM *dst_item);
void Item_AlignPosition(
    const XYZ_32 *vec, const ITEM *src_item, ITEM *dst_item);
int32_t Item_IsTriggerActive(ITEM *item);
int32_t Item_GetFrames(const ITEM *item, ANIM_FRAME *frmptr[], int32_t *rate);
BOUNDS_16 *Item_GetBoundsAccurate(const ITEM *item);
ANIM_FRAME *Item_GetBestFrame(const ITEM *item);
bool Item_IsNearItem(const ITEM *item, const XYZ_32 *pos, int32_t distance);

bool Item_IsSmashable(const ITEM *item);
int32_t Item_GetDistance(const ITEM *item, const XYZ_32 *target);
