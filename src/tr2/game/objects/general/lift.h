#pragma once

#include "global/types.h"

typedef struct __PACKING {
    int32_t start_height;
    int32_t wait_time;
} LIFT_INFO;

void __cdecl Lift_Initialise(int16_t item_num);
