#pragma once

#include "global/types.h"

typedef struct __PACKING {
    int32_t start_height;
    int32_t wait_time;
} LIFT_INFO;

void __cdecl Lift_Initialise(int16_t item_num);
void __cdecl Lift_Control(int16_t item_num);
void __cdecl Lift_FloorCeiling(
    const ITEM *item, int32_t x, int32_t y, int32_t z, int32_t *out_floor,
    int32_t *out_ceiling);
