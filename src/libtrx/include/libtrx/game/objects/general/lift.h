#pragma once

#include "../../types.h"

#define LIFT_NUM_FLOOR_SECTORS 4
#define LIFT_NUM_SECTORS 8

typedef struct {
    int32_t start_height;
    int32_t wait_time;
    bool is_moving;
    GAME_VECTOR linked[LIFT_NUM_SECTORS];
} LIFT_INFO;
