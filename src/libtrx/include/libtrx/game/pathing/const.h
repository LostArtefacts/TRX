#pragma once

#include "../const.h"

#define NO_BOX (-1)
#define BOX_ZONE(num) (((num) / STEP_L) - 1)
#if TR_VERSION == 1
    #define MAX_ZONES 2
#else
    #define MAX_ZONES 4
#endif
