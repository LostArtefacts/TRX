#pragma once

#include "../math/types.h"
#include "./types.h"

void Room_InitialiseRooms(int32_t num_rooms);
int32_t Room_GetCount(void);
ROOM *Room_Get(int32_t room_num);

void Room_InitialiseFlipStatus(void);
void Room_FlipMap(void);
bool Room_GetFlipStatus(void);
int32_t Room_GetFlipEffect(void);
void Room_SetFlipEffect(int32_t flip_effect);
int32_t Room_GetFlipTimer(void);
void Room_SetFlipTimer(int32_t flip_timer);
int32_t Room_GetFlipSlotFlags(int32_t slot_idx);
void Room_SetFlipSlotFlags(int32_t slot_idx, int32_t flags);

extern void Room_AlterFloorHeight(const ITEM *item, int32_t height);

int32_t Room_GetAdjoiningRooms(
    int16_t init_room_num, int16_t out_room_nums[], int32_t max_room_num_count);

void Room_ParseFloorData(const int16_t *floor_data);
void Room_PopulateSectorData(
    SECTOR *sector, const int16_t *floor_data, uint16_t start_index,
    uint16_t null_index);

int16_t Room_GetIndexFromPos(int32_t x, int32_t y, int32_t z);
int32_t Room_FindByPos(int32_t x, int32_t y, int32_t z);
BOUNDS_32 Room_GetWorldBounds(void);

SECTOR *Room_GetWorldSector(const ROOM *room, int32_t x_pos, int32_t z_pos);
SECTOR *Room_GetUnitSector(
    const ROOM *room, int32_t x_sector, int32_t z_sector);
