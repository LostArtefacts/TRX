#pragma once

// clang-format off
typedef enum {
    LF_SG_AIM_START           = 0,
    LF_SG_AIM_BEND            = 1,
    LF_SG_AIM_END             = 12,
    LF_SG_DRAW_START          = 13,
    LF_SG_DRAW_SFX            = 23,
    LF_SG_RECOIL_START        = 47,
    LF_SG_RECOILING           = 48,
    LF_SG_RECOIL_SFX          = 57,
    LF_SG_RECOIL_UNDRAW_RESET = 59,
    LF_SG_RECOIL_RESET_OG     = 60,
    LF_SG_RECOIL_RESET_FIX    = 63,
    LF_SG_RECOIL_END          = 79,
    LF_SG_UNDRAW_START        = 80,
    LF_SG_UNDRAW_SFX          = 100,
    LF_SG_UNDRAW_END          = 113,
    LF_SG_UNAIM_START         = 114,
    LF_SG_UNAIM_RAISE         = 126,
    LF_SG_UNAIM_END           = 127,
} LARA_SHOTGUN_ANIMATION_FRAME;
// clang-format on

// clang-format off
typedef enum {
    LF_G_AIM_START    = 0,
    LF_G_AIM_BEND     = 1,
    LF_G_AIM_EXTEND   = 3,
    LF_G_AIM_END      = 4,
    LF_G_UNDRAW_START = 5,
    LF_G_UNDRAW_BEND  = 6,
    LF_G_UNDRAW_END   = 12,
    LF_G_DRAW_START   = 13,
    LF_G_DRAW_END     = 23,
    LF_G_RECOIL_START = 24,
    LF_G_RECOIL_END   = 32,
} LARA_GUN_ANIMATION_FRAME;
// clang-format on
