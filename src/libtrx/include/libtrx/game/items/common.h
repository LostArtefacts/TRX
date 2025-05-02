#pragma once

#include "../anims.h"
#include "./types.h"

void Item_InitialiseItems(int32_t num_items);
ITEM *Item_Get(int16_t num);
int16_t Item_GetIndex(const ITEM *item);
ITEM *Item_Find(GAME_OBJECT_ID obj_id);

int32_t Item_GetLevelCount(void);
int32_t Item_GetTotalCount(void);

int16_t Item_GetNextActive(void);
int16_t Item_GetPrevActive(void);
void Item_SetPrevActive(int16_t item_num);

int16_t Item_Create(void);
int16_t Item_CreateLevelItem(void);
int16_t Item_Spawn(const ITEM *item, GAME_OBJECT_ID obj_id);

void Item_Initialise(int16_t item_num);
void Item_Kill(int16_t item_num);
void Item_RemoveActive(int16_t item_num);
void Item_RemoveDrawn(int16_t item_num);
void Item_AddActive(int16_t item_num);
void Item_UpdateRoom(int16_t item_num, int16_t room_num);

int32_t Item_GlobalReplace(
    GAME_OBJECT_ID src_obj_id, GAME_OBJECT_ID dst_obj_id);
bool Item_IsTriggerActive(ITEM *item);
