#pragma once

#include "global/const.h"
#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

extern int16_t *g_TriggerIndex;
extern int32_t g_FlipTimer;
extern int32_t g_FlipEffect;
extern int32_t g_FlipStatus;
extern int32_t g_FlipMapTable[MAX_FLIP_MAPS];

int16_t Room_GetTiltType(
    const SECTOR_INFO *sector, int32_t x, int32_t y, int32_t z);
int32_t Room_FindGridShift(int32_t src, int32_t dst);
void Room_GetNewRoom(int32_t x, int32_t y, int32_t z, int16_t room_num);
void Room_GetNearByRooms(
    int32_t x, int32_t y, int32_t z, int32_t r, int32_t h, int16_t room_num);
SECTOR_INFO *Room_GetSector(int32_t x, int32_t y, int32_t z, int16_t *room_num);
SECTOR_INFO *Room_GetPitSector(const SECTOR_INFO *sector, int32_t x, int32_t z);
int16_t Room_GetCeiling(
    const SECTOR_INFO *sector, int32_t x, int32_t y, int32_t z);
int16_t Room_GetHeight(
    const SECTOR_INFO *sector, int32_t x, int32_t y, int32_t z);
int16_t Room_GetWaterHeight(int32_t x, int32_t y, int32_t z, int16_t room_num);
int16_t Room_GetIndexFromPos(int32_t x, int32_t y, int32_t z);

void Room_AlterFloorHeight(ITEM_INFO *item, int32_t height);

void Room_ParseFloorData(const int16_t *floor_data);
void Room_TestTriggers(const ITEM_INFO *item);
void Room_FlipMap(void);
bool Room_IsOnWalkable(
    const SECTOR_INFO *sector, int32_t x, int32_t y, int32_t z,
    int32_t room_height);
