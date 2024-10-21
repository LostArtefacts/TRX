#pragma once

#include "types.h"

extern int32_t Room_GetTotalCount(void);
extern ROOM *Room_Get(int32_t room_num);

extern void Room_FlipMap(void);
extern bool Room_GetFlipStatus(void);

int32_t Room_GetAdjoiningRooms(
    int16_t init_room_num, int16_t out_room_nums[],
    const int32_t max_room_num_count);
