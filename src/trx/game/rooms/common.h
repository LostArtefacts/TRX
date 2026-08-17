#pragma once

#include <trx/core/handle.h>
#include <trx/core/math/types.h>
#include <trx/game/rooms/types.h>

void Room_InitialiseRooms(int32_t num_rooms);
int32_t Room_GetCount(void);
ROOM *Room_Get(int32_t room_num);

// A handle to the given room, and the room a handle still names or nullptr. A
// room is never recycled within a level, but the table is replaced whole at the
// next one; Room_InitialiseRooms starts a new epoch, so a handle kept across
// the change goes stale rather than naming a different room.
TRX_HANDLE Room_GetHandle(int32_t room_num);
ROOM *Room_FromHandle(TRX_HANDLE handle);
// The room's index, or NO_ROOM for one the level does not hold.
int32_t Room_GetIndex(const ROOM *room);

void Room_InitialiseFlipStatus(void);
void Room_FlipMap(void);
bool Room_GetFlipStatus(void);
int32_t Room_GetFlipEffect(void);
void Room_SetFlipEffect(int32_t flip_effect);
int32_t Room_GetFlipTimer(void);
void Room_SetFlipTimer(int32_t flip_timer);
void Room_IncrementFlipTimer(int32_t num_frames);
FLIP_SLOT *Room_GetFlipSlot(int32_t slot_idx);
// Applies a trigger to the slot; returns whether the slot's mask is complete.
bool Room_TriggerFlipSlot(int32_t slot_idx, const FLIP_TRIGGER *trigger);

int32_t Room_GetAdjoiningRooms(
    int16_t init_room_num, int16_t out_room_nums[], int32_t max_room_num_count);

int16_t Room_GetIndexFromPos(XYZ_32 pos);
int32_t Room_GetFlippedBaseRoom(int32_t room_num);
BOUNDS_32 Room_GetWorldBounds(void);

void Room_BuildOutsideTable(void);
int32_t Room_GetOutsideStatus(
    XYZ_32 pos, int16_t *out_room_num, int16_t *out_bbox_room_num);

bool Room_PointInside(const ROOM *room, XYZ_32 point);

// Returns whether the two rooms share space. Rooms that merely touch, such
// as neighbours meeting at a shared wall or stacked floor to ceiling, do not
// count as overlapping.
bool Room_CheckOverlap(int16_t room_num_0, int16_t room_num_1);

void Room_GetNearbyRooms(XYZ_32 pos, int32_t r, int32_t h, int16_t room_num);

bool Room_FindValidPos(XYZ_32 *out_pos, int16_t *out_room_num);
