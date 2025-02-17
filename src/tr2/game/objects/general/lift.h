#pragma once

#include "global/types.h"

typedef struct {
    int32_t start_height;
    int32_t wait_time;
} LIFT_INFO;

void Lift_Setup(void);
void Lift_Initialise(int16_t item_num);
void Lift_Control(int16_t item_num);
