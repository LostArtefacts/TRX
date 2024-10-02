#pragma once

#include "global/types.h"

#include <libtrx/game/rooms.h>

#include <stdint.h>

int16_t Room_GetIndexFromPos(int32_t x, int32_t y, int32_t z);
int32_t __cdecl Room_FindByPos(int32_t x, int32_t y, int32_t z);
int32_t __cdecl Room_FindGridShift(int32_t src, int32_t dst);
void __cdecl Room_GetNearbyRooms(
    int32_t x, int32_t y, int32_t z, int32_t r, int32_t h, int16_t room_num);
void __cdecl Room_GetNewRoom(int32_t x, int32_t y, int32_t z, int16_t room_num);
int16_t __cdecl Room_GetTiltType(
    const SECTOR *sector, int32_t x, int32_t y, int32_t z);
SECTOR *__cdecl Room_GetSector(
    int32_t x, int32_t y, int32_t z, int16_t *room_num);
int32_t __cdecl Room_GetWaterHeight(
    int32_t x, int32_t y, int32_t z, int16_t room_num);
int32_t __cdecl Room_GetHeight(
    const SECTOR *sector, int32_t x, int32_t y, int32_t z);
int32_t __cdecl Room_GetCeiling(
    const SECTOR *sector, int32_t x, int32_t y, int32_t z);
int16_t __cdecl Room_GetDoor(const SECTOR *sector);
void __cdecl Room_TestTriggers(const int16_t *fd, bool heavy);
void __cdecl Room_AlterFloorHeight(const ITEM *item, int32_t height);
void __cdecl Room_FlipMap(void);
void __cdecl Room_RemoveFlipItems(const ROOM *r);
void __cdecl Room_AddFlipItems(const ROOM *r);
void __cdecl Room_TriggerMusicTrack(int16_t value, int16_t flags, int16_t type);
