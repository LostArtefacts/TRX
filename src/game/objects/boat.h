#pragma once

#include "global/types.h"

#include <stdint.h>

typedef enum {
    BOAT_EMPTY = 0,
    BOAT_SET = 1,
    BOAT_MOVE = 2,
    BOAT_STOP = 3,
} BOAT_ANIM;

void SetupBoat(OBJECT_INFO *obj);
void BoatControl(int16_t item_num);
