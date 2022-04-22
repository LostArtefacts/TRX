#pragma once

#include "global/types.h"

void Item_InitialiseArray(int32_t num_items);
void Item_Kill(int16_t item_num);
int16_t Item_Create(void);
void Item_Initialise(int16_t item_num);
void Item_RemoveActive(int16_t item_num);
void Item_RemoveDrawn(int16_t item_num);
void Item_AddActive(int16_t item_num);
void Item_NewRoom(int16_t item_num, int16_t room_num);
void Item_UpdateRoom(ITEM_INFO *item, int32_t height);
int16_t Item_Spawn(ITEM_INFO *item, int16_t object_num);
int32_t Item_GlobalReplace(int32_t src_object_num, int32_t dst_object_num);

bool Item_IsNearItem(ITEM_INFO *item, PHD_3DPOS *pos, int32_t distance);
bool Item_TestBoundsCollide(
    ITEM_INFO *src_item, ITEM_INFO *dst_item, int32_t radius);
bool Item_TestPosition(
    ITEM_INFO *src_item, ITEM_INFO *dst_item, int16_t *bounds);
void Item_AlignPosition(
    ITEM_INFO *src_item, ITEM_INFO *dst_item, PHD_VECTOR *vec);
bool Item_MovePosition(
    ITEM_INFO *src_item, ITEM_INFO *dst_item, PHD_VECTOR *vec,
    int32_t velocity);
void Item_ShiftCol(ITEM_INFO *item, COLL_INFO *coll);
void Item_Translate(ITEM_INFO *item, int32_t x, int32_t y, int32_t z);

void Effect_InitialiseArray(void);
int16_t Effect_Create(int16_t room_num);
void KillEffect(int16_t fx_num);
void EffectNewRoom(int16_t fx_num, int16_t room_num);
