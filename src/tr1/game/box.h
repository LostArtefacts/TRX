#pragma once

#include "global/types.h"

#include <libtrx/game/pathing/box.h>
#include <libtrx/game/pathing/const.h>

bool Box_BadFloor(
    int32_t x, int32_t y, int32_t z, int16_t box_height, int16_t next_height,
    int16_t room_num, LOT_INFO *lot);
