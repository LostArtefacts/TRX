#pragma once

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

extern ITEM_INFO *g_Items;
extern int16_t g_NextItemActive;

void Item_InitialiseArray(int32_t num_items);
int32_t Item_GetTotalCount(void);
void Item_Control(void);
void Item_Kill(int16_t item_num);
int16_t Item_Create(void);
void Item_Initialise(int16_t item_num);
void Item_RemoveActive(int16_t item_num);
void Item_RemoveDrawn(int16_t item_num);
void Item_AddActive(int16_t item_num);
void Item_NewRoom(int16_t item_num, int16_t room_num);
void Item_UpdateRoom(ITEM_INFO *item, int32_t height);
int16_t Item_GetHeight(ITEM_INFO *item);
int16_t Item_GetWaterHeight(ITEM_INFO *item);
int16_t Item_Spawn(ITEM_INFO *item, int16_t object_num);
int32_t Item_GlobalReplace(int32_t src_object_num, int32_t dst_object_num);

bool Item_IsNearItem(
    const ITEM_INFO *item, const XYZ_32 *pos, int32_t distance);
bool Item_Test3DRange(int32_t x, int32_t y, int32_t z, int32_t range);
bool Item_TestBoundsCollide(
    ITEM_INFO *src_item, ITEM_INFO *dst_item, int32_t radius);
bool Item_TestPosition(
    ITEM_INFO *src_item, ITEM_INFO *dst_item, int16_t *bounds);
void Item_AlignPosition(ITEM_INFO *src_item, ITEM_INFO *dst_item, XYZ_32 *vec);
bool Item_MovePosition(
    ITEM_INFO *src_item, const ITEM_INFO *dst_item, const XYZ_32 *vec,
    int32_t velocity);
void Item_ShiftCol(ITEM_INFO *item, COLL_INFO *coll);
void Item_Translate(ITEM_INFO *item, int32_t x, int32_t y, int32_t z);
bool Item_Teleport(ITEM_INFO *item, int32_t x, int32_t y, int32_t z);

bool Item_TestAnimEqual(ITEM_INFO *item, int16_t anim_index);
void Item_SwitchToAnim(ITEM_INFO *item, int16_t anim_index, int16_t frame);
void Item_SwitchToObjAnim(
    ITEM_INFO *item, int16_t anim_index, int16_t frame,
    GAME_OBJECT_ID object_number);
void Item_Animate(ITEM_INFO *item);
bool Item_GetAnimChange(ITEM_INFO *item, ANIM_STRUCT *anim);
void Item_PlayAnimSFX(ITEM_INFO *item, int16_t *command, uint16_t flags);

bool Item_IsTriggerActive(ITEM_INFO *item);

int16_t *Item_GetBestFrame(const ITEM_INFO *item);
int16_t *Item_GetBoundsAccurate(const ITEM_INFO *item);
int32_t Item_GetFrames(const ITEM_INFO *item, int16_t *frmptr[], int32_t *rate);

void Item_TakeDamage(ITEM_INFO *item, int16_t damage, bool hit_status);
bool Item_TestFrameEqual(ITEM_INFO *item, int16_t frame);
bool Item_TestFrameRange(ITEM_INFO *item, int16_t start, int16_t end);
