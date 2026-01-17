#pragma once

#include <trx/game/items/types.h>
#include <trx/game/rooms/types.h>

typedef struct {
    bool drawn;
    bool shadow_drawn;
} OUTPUT_ITEM_BIND;

typedef struct {
    bool active;
    bool drawn;
} OUTPUT_ROOM_BIND;

void Output_Bind_ResetItems(void);
OUTPUT_ITEM_BIND *Output_Bind_GetItem(const ITEM *item);

void Output_Bind_ResetRooms(void);
OUTPUT_ROOM_BIND *Output_Bind_GetRoom(const ROOM *room);
