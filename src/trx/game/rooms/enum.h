#pragma once

typedef enum {
    HT_WALL = 0,
    HT_SMALL_SLOPE = 1,
    HT_BIG_SLOPE = 2,
    HT_DIAGONAL = 3,
    HT_SPLIT_TRI = 4,
} HEIGHT_TYPE;

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
    FT_CLIMB = 6,
    FT_FLOOR_NWSE_SOLID = 7,
    FT_FLOOR_NESW_SOLID = 8,
    FT_ROOF_NWSE_SOLID = 9,
    FT_ROOF_NESW_SOLID = 10,
    FT_FLOOR_NWSE_PORTAL_SW = 11,
    FT_FLOOR_NWSE_PORTAL_NE = 12,
    FT_FLOOR_NESW_PORTAL_SE = 13,
    FT_FLOOR_NESW_PORTAL_NW = 14,
    FT_ROOF_NWSE_PORTAL_SW = 15,
    FT_ROOF_NWSE_PORTAL_NE = 16,
    FT_ROOF_NESW_PORTAL_NW = 17,
    FT_ROOF_NESW_PORTAL_SE = 18,
    FT_MONKEY = 19,
    FT_MINE_CART_LEFT = 20,
    FT_MINE_CART_RIGHT = 21,
} FLOOR_TYPE;

typedef enum {
    SPLIT_NONE,
    SPLIT_NWSE_SOLID,
    SPLIT_NESW_SOLID,
    SPLIT_NWSE_PORTAL_SW,
    SPLIT_NWSE_PORTAL_NE,
    SPLIT_NESW_PORTAL_SE,
    SPLIT_NESW_PORTAL_NW,
} SPLIT_TYPE;

typedef enum {
    SURFACE_FLOOR,
    SURFACE_CEILING,
} SURFACE_TYPE;

typedef enum {
    TO_ITEM,
    TO_CAMERA,
    TO_SINK,
    TO_FLIP_MAP,
    TO_FLIP_ON,
    TO_FLIP_OFF,
    TO_TARGET,
    TO_FINISH,
    TO_MUSIC,
    TO_FLIP_EFFECT,
    TO_SECRET,
    TO_BODY_BAG,
    TO_FLYBY,
    TO_NUMBER_OF,
} TRIGGER_OBJECT;

typedef enum {
    TT_TRIGGER,
    TT_PAD,
    TT_SWITCH,
    TT_KEY,
    TT_PICKUP,
    TT_HEAVY,
    TT_ANTIPAD,
    TT_COMBAT,
    TT_DUMMY,
    TT_ANTITRIGGER,
    TT_HEAVY_SWITCH,
    TT_HEAVY_ANTITRIGGER,
    TT_MONKEY,
    // TR5 uses TT_SKELETON and TT_TIGHTROPE. Numbering reserved to align with
    // level compilers.
    TT_CROUCH = 15,
    TT_CLIMB,
} TRIGGER_TYPE;

typedef enum {
    // clang-format off
    LADDER_NONE    = 0,
    LADDER_NORTH   = 1 << 0,
    LADDER_EAST    = 1 << 1,
    LADDER_SOUTH   = 1 << 2,
    LADDER_WEST    = 1 << 3,
    LADDER_CEILING = 1 << 4,
    // clang-format on
} LADDER_DIRECTION;

typedef enum {
    MINE_CART_NONE,
    MINE_CART_LEFT,
    MINE_CART_RIGHT,
    MINE_CART_STOP,
    NUM_MINE_CART_TYPES,
} MINE_CART_TYPE;
