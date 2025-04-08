#pragma once

#include "../../utils.h"

#define FRONT_ARC DEG_90
#define UNIT_SHADOW 256

#define CREATURE_STALK_DIST (3 * WALL_L) // = 3072
#define CREATURE_ESCAPE_DIST (5 * WALL_L) // = 5120
#define CREATURE_TARGET_DIST (4 * WALL_L) // = 4096

#define CREATURE_MISS_CHANCE 0x2000
#if TR_VERSION >= 2
    #define CREATURE_SHOOT_RANGE SQUARE(WALL_L * 8) // = 0x4000000 = 67108864
#else
    #define CREATURE_SHOOT_RANGE SQUARE(WALL_L * 7) // = 51380224
#endif
