#pragma once

#include "../const.h"

#define NO_BOX (-1)
#define BOX_ZONE(num) (((num) / STEP_L) - 1)
#define LOT_SLOT_COUNT 32
#if TR_VERSION == 1
    #define MAX_ZONES 2
#else
    #define MAX_ZONES 4
#endif

#define BOX_BLOCKED 0x4000
#define BOX_BLOCKED_SEARCH 0x8000
#define BOX_BLOCKABLE 0x8000
