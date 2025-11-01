#pragma once

typedef enum {
    // clang-format off
    DS_CARRIED   = 0,
    DS_FALLING   = 1,
    DS_DROPPED   = 2,
    DS_COLLECTED = 3,
    // clang-format on
} DROP_STATUS;

typedef enum {
    // clang-format off
    IF_ONE_SHOT  = 0x0100,
    IF_CODE_BITS = 0x3E00,
    IF_REVERSE   = 0x4000,
    IF_INVISIBLE = 0x0100,
    IF_KILLED    = 0x8000,
    // clang-format on
} ITEM_FLAG;

typedef enum {
    // clang-format off
    IS_INACTIVE    = 0,
    IS_ACTIVE      = 1,
    IS_DEACTIVATED = 2,
    IS_INVISIBLE   = 3,
    // clang-format on
} ITEM_STATUS;
