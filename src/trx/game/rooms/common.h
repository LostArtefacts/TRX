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
// Swaps the rooms of one flip group with their pairs. TR4 splits a level's
// flip pairs into groups and moves one at a time; earlier games have only
// group 0, which is what every room of theirs belongs to.
void Room_FlipMap(int32_t group);

// Whether the group that moved last is showing its pairs.
//
// The level's data is written for a world with two states rather than a world
// of groups: there is one set of pathing zones for each, and an ambient sound
// source says it belongs to one or the other. Neither can name a group, so the
// engine keeps this one answer for them, and a flip anywhere in the level sets
// it. Where a level uses several groups it is therefore not "is anything
// flipped", and a caller that means one group should ask about that group.
bool Room_GetFlipStatus(void);

// Whether the given group is showing its pairs.
bool Room_GetFlipGroupStatus(int32_t group);

// The group a flip slot moves. A TR4 trigger names the group in the same
// number it uses for the slot, where a TR1-3 trigger means the slot alone and
// moves every pair in the level. A level that puts no room in a group is read
// the TR1-3 way.
int32_t Room_GetFlipGroup(int32_t flip_slot);

// Puts back what a savegame recorded, once its groups have been replayed. The
// groups alone do not say which of them moved last, and that is what the zones
// and the sound sources read.
void Room_SetFlipStatus(bool status);
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

// Builds the per-room overlap map after room bounds are finalized. A room and
// its flip alternative occupy the same space by design and do not count.
void Room_InitialiseOverlapMap(void);

// Returns whether the room shares space with any other non-flip room.
bool Room_IsOverlapping(int16_t room_num);

void Room_GetNearbyRooms(XYZ_32 pos, int32_t r, int32_t h, int16_t room_num);

bool Room_FindValidPos(XYZ_32 *out_pos, int16_t *out_room_num);

// Attempts direct traversal first with Room_GetSector, then individual axis
// travseral to resolve cases where direct movement is not possible. Returns
// NO_ROOM if all attempts fail.
int16_t Room_FindByTraversal(
    XYZ_32 old_pos, XYZ_32 new_pos, int16_t start_room);
