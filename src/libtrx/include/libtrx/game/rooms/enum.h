#pragma once

typedef enum {
    HT_WALL = 0,
    HT_SMALL_SLOPE = 1,
    HT_BIG_SLOPE = 2,
} HEIGHT_TYPE;

typedef enum {
    RF_UNDERWATER = 0x01,
    RF_OUTSIDE = 0x08,
    RF_DYNAMIC_LIT = 0x10,
    RF_NOT_INSIDE = 0x20,
    RF_INSIDE = 0x40,
} ROOM_FLAG;

typedef enum {
    RLM_NORMAL = 0,
    RLM_FLICKER = 1,
    RLM_GLOW = 2,
    RLM_SUNSET = 3,
    RLM_NUMBER_OF = 4,
} ROOM_LIGHT_MODE;

typedef enum {
    RFS_NONE = 0,
    RFS_UNFLIPPED = 1,
    RFS_FLIPPED = 2,
} ROOM_FLIP_STATUS;

typedef enum {
    FT_FLOOR = 0,
    FT_DOOR = 1,
    FT_TILT = 2,
    FT_ROOF = 3,
    FT_TRIGGER = 4,
    FT_LAVA = 5,
#if TR_VERSION == 2
    FT_CLIMB = 6,
#endif
} FLOOR_TYPE;

typedef enum {
    TO_OBJECT = 0,
    TO_CAMERA = 1,
    TO_SINK = 2,
    TO_FLIPMAP = 3,
    TO_FLIPON = 4,
    TO_FLIPOFF = 5,
    TO_TARGET = 6,
    TO_FINISH = 7,
    TO_CD = 8,
    TO_FLIPEFFECT = 9,
    TO_SECRET = 10,
#if TR_VERSION == 2
    TO_BODY_BAG = 11,
#endif
} TRIGGER_OBJECT;

typedef enum {
    TT_TRIGGER = 0,
    TT_PAD = 1,
    TT_SWITCH = 2,
    TT_KEY = 3,
    TT_PICKUP = 4,
    TT_HEAVY = 5,
    TT_ANTIPAD = 6,
    TT_COMBAT = 7,
    TT_DUMMY = 8,
#if TR_VERSION == 2
    TT_ANTITRIGGER = 9,
#endif
} TRIGGER_TYPE;

#if TR_VERSION == 2
typedef enum {
    LADDER_NONE = 0,
    LADDER_NORTH = 1 << 0,
    LADDER_EAST = 1 << 1,
    LADDER_SOUTH = 1 << 2,
    LADDER_WEST = 1 << 3,
} LADDER_DIRECTION;
#endif
