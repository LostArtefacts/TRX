#pragma once

#include "global/types.h"

#include <stdint.h>

extern ITEM *g_Items;
extern int16_t g_NextItemActive;

void Item_InitialiseArray(int32_t num_items);
void Item_Control(void);
void Item_Kill(int16_t item_num);
void Item_Initialise(int16_t item_num);
void Item_RemoveActive(int16_t item_num);
void Item_RemoveDrawn(int16_t item_num);
void Item_AddActive(int16_t item_num);
void Item_NewRoom(int16_t item_num, int16_t room_num);
void Item_UpdateRoom(ITEM *item, int32_t height);
int16_t Item_GetHeight(ITEM *item);
int16_t Item_GetWaterHeight(ITEM *item);
int16_t Item_Spawn(const ITEM *item, GAME_OBJECT_ID obj_id);
int32_t Item_GlobalReplace(
    GAME_OBJECT_ID src_obj_id, GAME_OBJECT_ID dst_obj_id);

bool Item_IsNearItem(const ITEM *item, const XYZ_32 *pos, int32_t distance);
bool Item_Test3DRange(int32_t x, int32_t y, int32_t z, int32_t range);
bool Item_TestBoundsCollide(ITEM *src_item, ITEM *dst_item, int32_t radius);
bool Item_TestPosition(
    const ITEM *src_item, const ITEM *dst_item, const OBJECT_BOUNDS *bounds);
void Item_AlignPosition(ITEM *src_item, ITEM *dst_item, XYZ_32 *vec);
bool Item_MovePosition(
    ITEM *src_item, const ITEM *dst_item, const XYZ_32 *vec, int32_t velocity);
void Item_ShiftCol(ITEM *item, COLL_INFO *coll);
int32_t Item_GetDistance(const ITEM *item, const XYZ_32 *target);

bool Item_IsTriggerActive(ITEM *item);

ANIM_FRAME *Item_GetBestFrame(const ITEM *item);
const BOUNDS_16 *Item_GetBoundsAccurate(const ITEM *item);
int32_t Item_GetFrames(const ITEM *item, ANIM_FRAME *frmptr[], int32_t *rate);

void Item_TakeDamage(ITEM *item, int16_t damage, bool hit_status);
