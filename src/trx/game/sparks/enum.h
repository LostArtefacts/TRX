#pragma once

enum {
    // clang-format off
    SPARK_F_NONE          = 0x0,
    SPARK_F_SCALE         = 0x2,
    SPARK_F_BLOOD         = 0x4,
    SPARK_F_SPRITE        = 0x8,
    SPARK_F_ROTATE        = 0x10,
    SPARK_F_FX            = 0x40,
    SPARK_F_ITEM          = 0x80,
    SPARK_F_OUTSIDE       = 0x100,
    SPARK_F_ALT_SPRITE    = 0x200,
    SPARK_F_ATTACHED_POS  = 0x400,
    SPARK_F_UNDERWATER    = 0x800,
    SPARK_F_ATTACHED_NODE = 0x1000,
    SPARK_F_GREEN         = 0x2000,
    // clang-format on
};

typedef enum {
    // clang-format off
    SPARK_TYPE_EXPLOSION    = 0,
    SPARK_TYPE_SMALL_SPLASH = 4,
    SPARK_TYPE_BIG_SPLASH   = 8,
    SPARK_TYPE_RIPPLE       = 9,
    SPARK_TYPE_FOOTPRINT    = 10,
    SPARK_TYPE_PARTICLE     = SPARK_TYPE_FOOTPRINT,
    SPARK_TYPE_SHIELD       = 11,
    SPARK_TYPE_ROPE         = 19,
    SPARK_TYPE_DRIVE        = 20,
    SPARK_TYPE_REVERSE      = 21,
    SPARK_TYPE_MENU_1       = 22,
    SPARK_TYPE_MENU_2       = 23,
    SPARK_TYPE_MENU_3       = 24,
    SPARK_TYPE_MENU_4       = 25,
    SPARK_TYPE_MENU_5       = 26,
    SPARK_TYPE_MENU_6       = 27,
    SPARK_TYPE_MENU_7       = 28,
    SPARK_TYPE_MENU_8       = 29,
    SPARK_TYPE_MENU_9       = 30,
    SPARK_TYPE_UNKNOWN_1    = 31, // OG TR3 13
    SPARK_TYPE_UNKNOWN_2    = 32, // OG TR3 14
    SPARK_TYPE_UNKNOWN_3    = 33, // OG TR3 15
    SPARK_TYPE_UNKNOWN_4    = 34, // OG TR3 16
    SPARK_TYPE_LENS_FLARE_1 = 35, // OG TR4 11
    SPARK_TYPE_RICOCHET     = 36, // OG TR4 12
    SPARK_TYPE_UNKNOWN_7    = 37, // OG TR4 13
    SPARK_TYPE_UNKNOWN_8    = 38, // OG TR4 14
    SPARK_TYPE_BLOOD        = 39, // OG TR4 15
    SPARK_TYPE_UNKNOWN_10   = 40, // OG TR4 28
    SPARK_TYPE_LENS_FLARE_2 = 41, // OG TR4 29
    SPARK_TYPE_LENS_FLARE_3 = 42, // OG TR4 30
    SPARK_TYPE_LENS_FLARE_4 = 43, // OG TR4 31
    SPARK_TYPE_LENS_FLARE_5 = 44, // OG TR4 32
    // clang-format on
} SPARK_SPRITE_TYPE;
