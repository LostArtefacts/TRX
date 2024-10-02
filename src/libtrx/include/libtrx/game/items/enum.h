#pragma once

#if TR_VERSION == 1
typedef enum {
    DS_CARRIED = 0,
    DS_FALLING = 1,
    DS_DROPPED = 2,
    DS_COLLECTED = 3,
} DROP_STATUS;
#endif

// clang-format off
typedef enum {
    IF_ONE_SHOT  = 0x0100,
    IF_CODE_BITS = 0x3E00,
    IF_REVERSE   = 0x4000,
    IF_INVISIBLE = 0x0100,
    IF_KILLED    = 0x8000,
} ITEM_FLAG;


typedef enum {
    IS_INACTIVE    = 0,
    IS_ACTIVE      = 1,
    IS_DEACTIVATED = 2,
    IS_INVISIBLE   = 3,
} ITEM_STATUS;

// clang-format on
