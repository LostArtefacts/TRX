#pragma once

#include "global/types.h"

#include <stdint.h>

void Blood_Setup(OBJECT_INFO *obj);
void Blood_Control(int16_t fx_num);

int16_t Blood_Spawn(
    int32_t x, int32_t y, int32_t z, int16_t speed, int16_t direction,
    int16_t room_num);
