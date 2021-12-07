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

void SetupCabin(OBJECT_INFO *obj);
void CabinControl(int16_t item_num);
