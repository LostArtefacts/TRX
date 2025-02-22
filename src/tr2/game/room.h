#pragma once

#include "global/types.h"

#include <libtrx/game/rooms.h>

#include <stdint.h>

int32_t Room_FindGridShift(int32_t src, int32_t dst);
void Room_GetNearbyRooms(
    int32_t x, int32_t y, int32_t z, int32_t r, int32_t h, int16_t room_num);
void Room_GetNewRoom(int32_t x, int32_t y, int32_t z, int16_t room_num);
int16_t Room_GetTiltType(const SECTOR *sector, int32_t x, int32_t y, int32_t z);

// TODO: poor abstraction
void Room_InitCinematic(void);

int32_t Room_GetWaterHeight(int32_t x, int32_t y, int32_t z, int16_t room_num);

void Room_TestTriggers(const ITEM *item);
void Room_TestSectorTrigger(const ITEM *item, const SECTOR *sector);
