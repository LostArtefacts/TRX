#pragma once

#include "global/types.h"

#include <stdint.h>

void InitialiseItemArray(int32_t num_items);
void KillItem(int16_t item_num);
int16_t CreateItem(void);
void InitialiseItem(int16_t item_num);
void RemoveActiveItem(int16_t item_num);
void RemoveDrawnItem(int16_t item_num);
void AddActiveItem(int16_t item_num);
void ItemNewRoom(int16_t item_num, int16_t room_num);
void Item_UpdateRoom(ITEM_INFO *item, int32_t height);
int16_t SpawnItem(ITEM_INFO *item, int16_t object_num);
int32_t GlobalItemReplace(int32_t src_object_num, int32_t dst_object_num);

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

void InitialiseFXArray(void);
int16_t CreateEffect(int16_t room_num);
void KillEffect(int16_t fx_num);
void EffectNewRoom(int16_t fx_num, int16_t room_num);
