#pragma once

#include "../math/types.h"
#include "./types.h"

void Room_InitialiseRooms(int32_t num_rooms);
int32_t Room_GetCount(void);
ROOM *Room_Get(int32_t room_num);
int32_t Room_GetNumber(const ROOM *room);

void Room_InitialiseFlipStatus(void);
void Room_FlipMap(void);
bool Room_GetFlipStatus(void);
int32_t Room_GetFlipEffect(void);
void Room_SetFlipEffect(int32_t flip_effect);
int32_t Room_GetFlipTimer(void);
void Room_SetFlipTimer(int32_t flip_timer);
void Room_IncrementFlipTimer(int32_t num_frames);
int32_t Room_GetFlipSlotFlags(int32_t slot_idx);
void Room_SetFlipSlotFlags(int32_t slot_idx, int32_t flags);

int32_t Room_GetAdjoiningRooms(
    int16_t init_room_num, int16_t out_room_nums[], int32_t max_room_num_count);

int16_t Room_GetIndexFromPos(int32_t x, int32_t y, int32_t z);
int32_t Room_GetFlippedBaseRoom(int32_t room_num);
BOUNDS_32 Room_GetWorldBounds(void);

bool Room_CheckOverlap(int16_t room_num_0, int16_t room_num_1);
void Room_GetNearbyRooms(
    int32_t x, int32_t y, int32_t z, int32_t r, int32_t h, int16_t room_num);

bool Room_FindValidPos(XYZ_32 *out_pos, int16_t *out_room_num);
