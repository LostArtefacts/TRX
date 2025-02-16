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

SECTOR *Room_GetPitSector(const SECTOR *sector, int32_t x, int32_t z);
SECTOR *Room_GetSkySector(const SECTOR *sector, int32_t x, int32_t z);
SECTOR *Room_GetSector(int32_t x, int32_t y, int32_t z, int16_t *room_num);

int32_t Room_GetWaterHeight(int32_t x, int32_t y, int32_t z, int16_t room_num);
int32_t Room_GetHeight(const SECTOR *sector, int32_t x, int32_t y, int32_t z);
int32_t Room_GetCeiling(const SECTOR *sector, int32_t x, int32_t y, int32_t z);

void Room_TestTriggers(const ITEM *item);
void Room_TestSectorTrigger(const ITEM *item, const SECTOR *sector);

// TODO: eliminate
int16_t Room_Legacy_GetDoor(const SECTOR *sector);
void Room_Legacy_TestTriggers(const int16_t *fd, bool heavy);
void Room_Legacy_TriggerMusicTrack(int16_t value, int16_t flags, int16_t type);
