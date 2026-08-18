#pragma once

#include <trx/game/rooms/types.h>

BOUNDS_32 Room_GetRoomBounds(const ROOM *room);

// Returns whether the box reaches through the portal into the room behind it.
// Wall portals match on full bounds intersection. Floor and ceiling portals
// match when the box stands over the opening and crosses the portal plane.
// Portal bounds must already be finalized.
bool Room_BoundsReachPortal(const BOUNDS_32 *bounds, const PORTAL *portal);
SECTOR *Room_GetSector(XYZ_32 pos, int16_t *room_num);
SECTOR *Room_GetSectorOnWalkable(XYZ_32 pos, int16_t *room_num);
SECTOR *Room_GetWorldSector(const ROOM *room, int32_t x_pos, int32_t z_pos);
SECTOR *Room_GetUnitSector(
    const ROOM *room, int32_t x_sector, int32_t z_sector);
SECTOR *Room_GetPitSector(const SECTOR *sector, int32_t x, int32_t z);
SECTOR *Room_GetSkySector(const SECTOR *sector, int32_t x, int32_t z);

void Room_SetAbyssHeight(int32_t height);
bool Room_IsAbyssHeight(int32_t height);

HEIGHT_TYPE Room_GetHeightType(void);
XZ_16 Room_GetTiltType(const SECTOR *sector, XYZ_32 pos);

int32_t Room_GetHeight(const SECTOR *sector, XYZ_32 pos);
int32_t Room_GetHeightEx(
    const SECTOR *sector, XYZ_32 pos, bool fix_tilts, int16_t ignore_item_num);
int32_t Room_GetCeiling(const SECTOR *sector, XYZ_32 pos);
int32_t Room_GetCeilingEx(const SECTOR *sector, XYZ_32 pos, bool fix_tilts);
int32_t Room_GetFloorHeightForSector(
    const SECTOR *sector, int32_t x, int32_t z, bool fix_tilts);

typedef struct {
    // Read the surface at (x,z) rather than the room's own plane.
    bool fix_tilts;
    // Report NO_HEIGHT where a solid ceiling caps the water. Without it, that
    // ceiling is reported as the surface, which is what the OG does.
    bool require_air_above;
} ROOM_WATER_HEIGHT_ARGS;

int32_t Room_GetWaterHeight(XYZ_32 pos, int16_t room_num);
int32_t Room_GetWaterHeightEx(
    XYZ_32 pos, int16_t room_num, ROOM_WATER_HEIGHT_ARGS args);

int32_t Room_FindGridShift(int32_t src, int32_t dst);

bool Room_IsOnWalkable(
    const SECTOR *sector, XYZ_32 pos, int32_t room_height,
    int16_t ignore_item_num);
