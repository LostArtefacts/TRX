#pragma once

typedef enum {
    // clang-format off
    IDF_NONE          = 0,
    IDF_NO_HIT_STATUS = 1 << 0,
    // clang-format on
} ITEM_DAMAGE_FLAGS;

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
    IF_ONE_SHOT_SWITCH      = 0x0040,
    IF_ONE_SHOT_ANTITRIGGER = 0x0080,
    IF_ONE_SHOT             = 0x0100,
    IF_CODE_BITS            = 0x3E00,
    IF_REVERSE              = 0x4000,
    IF_INVISIBLE            = 0x0100,
    IF_DESTROYED            = 0x8000,
    // clang-format on
} ITEM_FLAG;

// The flag operation an item trigger performs, decoupled from the floordata
// TRIGGER_TYPE that a sector carries. Room_Handle maps a TRIGGER_TYPE onto one
// of these at the rooms->items boundary; the item code and every trigger_func
// only ever see the kind. Distinct values are kept where the OG behavior
// diverges: a plain switch spends differently from a heavy switch, and
// falling_block and pickup single out the heavy and switch cases.
typedef enum {
    ITEM_TRIGGER_NORMAL, // TT_TRIGGER, TT_PAD, TT_KEY, TT_PICKUP, TT_COMBAT,
                         // TT_DUMMY, TT_MONKEY, TT_CROUCH, TT_CLIMB
    ITEM_TRIGGER_HEAVY, // TT_HEAVY
    ITEM_TRIGGER_SWITCH, // TT_SWITCH
    ITEM_TRIGGER_HEAVY_SWITCH, // TT_HEAVY_SWITCH
    ITEM_TRIGGER_ANTI, // TT_ANTIPAD, TT_ANTITRIGGER, TT_HEAVY_ANTITRIGGER
} ITEM_TRIGGER_KIND;

typedef enum {
    // clang-format off
    AI_GUARD    = 1 << 0,
    AI_AMBUSH   = 1 << 1,
    AI_PATROL_1 = 1 << 2,
    AI_MODIFY   = 1 << 3,
    AI_FOLLOW   = 1 << 4,
    // clang-format on
} AI_BITS;
