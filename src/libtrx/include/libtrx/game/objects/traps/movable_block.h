#pragma once

#include "../../rooms.h"

void MovableBlock_Initialise(int16_t item_num);
void MovableBlock_UpdateRotation(ITEM *item, int16_t rot_y);
void MovableBlock_SetupFloor(void);
void MovableBlock_HandleFlipMap(ROOM_FLIP_STATUS flip_status);
