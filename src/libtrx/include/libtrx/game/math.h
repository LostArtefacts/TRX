#pragma once

#include "types.h"

#include <stdint.h>

#pragma pack(push, 1)

typedef struct __PACKING {
    int32_t x;
    int32_t y;
    int32_t z;
} XYZ_32;

typedef struct __PACKING {
    int16_t x;
    int16_t y;
    int16_t z;
} XYZ_16;

typedef enum {
    DIR_UNKNOWN = -1,
    DIR_NORTH = 0,
    DIR_EAST = 1,
    DIR_SOUTH = 2,
    DIR_WEST = 3,
} DIRECTION;

#if TR_VERSION == 1
typedef struct __PACKING {
    XYZ_16 min;
    XYZ_16 max;
} BOUNDS_16;

#elif TR_VERSION == 2
typedef struct __PACKING {
    int16_t min_x;
    int16_t max_x;
    int16_t min_y;
    int16_t max_y;
    int16_t min_z;
    int16_t max_z;
} BOUNDS_16;
#endif

#pragma pack(pop)
