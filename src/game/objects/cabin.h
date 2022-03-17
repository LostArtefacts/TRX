#pragma once

#include "global/types.h"

#include <stdint.h>

typedef enum {
    CABIN_START = 0,
    CABIN_DROP1 = 1,
    CABIN_DROP2 = 2,
    CABIN_DROP3 = 3,
    CABIN_FINISH = 4,
} CABIN_ANIM;

void Cabin_Setup(OBJECT_INFO *obj);
void Cabin_Control(int16_t item_num);
