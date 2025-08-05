#pragma once

#include "global/types.h"

void Room_GetBounds(void);
void Room_SetBounds(
    const int16_t *obj_ptr, int32_t room_num, const ROOM *parent);
void Room_DrawAllRooms(int16_t current_room);
void Room_DrawSingleRoomGeometry(int16_t room_num);
void Room_DrawSingleRoomObjects(int16_t room_num);
