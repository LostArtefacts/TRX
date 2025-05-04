#pragma once

#include "global/types.h"

#include <libtrx/game/rooms.h>

#include <stdint.h>

void Room_GetNewRoom(int32_t x, int32_t y, int32_t z, int16_t room_num);

// TODO: poor abstraction
void Room_InitCinematic(void);
