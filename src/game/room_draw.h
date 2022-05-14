#pragma once

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

bool Room_SetBounds(int16_t *objptr, int16_t room_num, ROOM_INFO *parent);
void Room_GetBounds(int16_t room_num);
