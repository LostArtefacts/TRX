#pragma once

#include "global/const.h"
#include "global/types.h"

#include <libtrx/game/rooms.h>

#include <stdint.h>

extern int32_t g_FlipMapTable[MAX_FLIP_MAPS];

int16_t Room_GetTiltType(const SECTOR *sector, int32_t x, int32_t y, int32_t z);
int32_t Room_FindGridShift(int32_t src, int32_t dst);
void Room_GetNewRoom(int32_t x, int32_t y, int32_t z, int16_t room_num);
void Room_GetNearByRooms(
    int32_t x, int32_t y, int32_t z, int32_t r, int32_t h, int16_t room_num);
SECTOR *Room_GetSector(int32_t x, int32_t y, int32_t z, int16_t *room_num);
SECTOR *Room_GetPitSector(const SECTOR *sector, int32_t x, int32_t z);
int16_t Room_GetCeiling(const SECTOR *sector, int32_t x, int32_t y, int32_t z);
int16_t Room_GetHeight(const SECTOR *sector, int32_t x, int32_t y, int32_t z);
int16_t Room_GetWaterHeight(int32_t x, int32_t y, int32_t z, int16_t room_num);

void Room_AlterFloorHeight(ITEM *item, int32_t height);

void Room_TestTriggers(const ITEM *item);
void Room_TestSectorTrigger(const ITEM *item, const SECTOR *sector);
bool Room_IsOnWalkable(
    const SECTOR *sector, int32_t x, int32_t y, int32_t z, int32_t room_height);
