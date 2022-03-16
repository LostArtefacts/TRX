#pragma once

#include "global/types.h"

#include <stdint.h>

int32_t ControlPhase(int32_t nframes, GAMEFLOW_LEVEL_TYPE level_type);
void AnimateItem(ITEM_INFO *item);
int32_t GetChange(ITEM_INFO *item, ANIM_STRUCT *anim);
void TranslateItem(ITEM_INFO *item, int32_t x, int32_t y, int32_t z);
FLOOR_INFO *GetFloor(int32_t x, int32_t y, int32_t z, int16_t *room_num);
int16_t GetWaterHeight(int32_t x, int32_t y, int32_t z, int16_t room_num);
int16_t GetHeight(FLOOR_INFO *floor, int32_t x, int32_t y, int32_t z);
void RefreshCamera(int16_t type, int16_t *data);
void TestTriggers(int16_t *data, int32_t heavy);
int32_t TriggerActive(ITEM_INFO *item);
int16_t GetCeiling(FLOOR_INFO *floor, int32_t x, int32_t y, int32_t z);
int16_t GetDoor(FLOOR_INFO *floor);
int32_t LOS(GAME_VECTOR *start, GAME_VECTOR *target);
int32_t zLOS(GAME_VECTOR *start, GAME_VECTOR *target);
int32_t xLOS(GAME_VECTOR *start, GAME_VECTOR *target);
int32_t ClipTarget(GAME_VECTOR *start, GAME_VECTOR *target, FLOOR_INFO *floor);
void FlipMap(void);
void RemoveRoomFlipItems(ROOM_INFO *r);
void AddRoomFlipItems(ROOM_INFO *r);

bool Control_Pause(void);
