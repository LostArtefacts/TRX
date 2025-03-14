#pragma once

// clang-format off
typedef enum {
    LWS_ABOVE_WATER  = 0,
    LWS_UNDERWATER   = 1,
    LWS_SURFACE      = 2,
    LWS_CHEAT        = 3,
    LWS_WADE         = 4,
} LARA_WATER_STATE;

typedef enum {
    LGS_ARMLESS    = 0,
    LGS_HANDS_BUSY = 1,
    LGS_DRAW       = 2,
    LGS_UNDRAW     = 3,
    LGS_READY      = 4,
    LGS_SPECIAL    = 5,
} LARA_GUN_STATE;

typedef enum {
    LM_HIPS      = 0,
    LM_THIGH_L   = 1,
    LM_CALF_L    = 2,
    LM_FOOT_L    = 3,
    LM_THIGH_R   = 4,
    LM_CALF_R    = 5,
    LM_FOOT_R    = 6,
    LM_TORSO     = 7,
    LM_UARM_R    = 8,
    LM_LARM_R    = 9,
    LM_HAND_R    = 10,
    LM_UARM_L    = 11,
    LM_LARM_L    = 12,
    LM_HAND_L    = 13,
    LM_HEAD      = 14,
    LM_FIRST     = LM_HIPS,
    LM_NUMBER_OF = 15,
} LARA_MESH;

typedef enum {
    LS_WALK          = 0,
    LS_RUN           = 1,
    LS_STOP          = 2,
    LS_JUMP_FORWARD  = 3,
    LS_POSE          = 4,
    LS_FAST_BACK     = 5,
    LS_TURN_RIGHT    = 6,
    LS_TURN_LEFT     = 7,
    LS_DEATH         = 8,
    LS_FAST_FALL     = 9,
    LS_HANG          = 10,
    LS_REACH         = 11,
    LS_SPLAT         = 12,
    LS_TREAD         = 13,
    LS_LAND          = 14,
    LS_COMPRESS      = 15,
    LS_BACK          = 16,
    LS_SWIM          = 17,
    LS_GLIDE         = 18,
#if TR_VERSION      >= 2
    LS_NULL          = 19,
#elif TR_VERSION    == 1
    LS_CLIMB_UP      = 19,
#endif
    LS_FAST_TURN     = 20,
    LS_STEP_RIGHT    = 21,
    LS_STEP_LEFT     = 22,
    LS_HIT           = 23,
    LS_SLIDE         = 24,
    LS_JUMP_BACK     = 25,
    LS_JUMP_RIGHT    = 26,
    LS_JUMP_LEFT     = 27,
    LS_JUMP_UP       = 28,
    LS_FALL_BACK     = 29,
    LS_HANG_LEFT     = 30,
    LS_HANG_RIGHT    = 31,
    LS_SLIDE_BACK    = 32,
    LS_SURF_TREAD    = 33,
    LS_SURF_SWIM     = 34,
    LS_DIVE          = 35,
    LS_PUSH_BLOCK    = 36,
    LS_PULL_BLOCK    = 37,
    LS_PP_READY      = 38,
    LS_PICKUP        = 39,
    LS_SWITCH_ON     = 40,
    LS_SWITCH_OFF    = 41,
    LS_USE_KEY       = 42,
    LS_USE_PUZZLE    = 43,
    LS_UW_DEATH      = 44,
    LS_ROLL          = 45,
    LS_SPECIAL       = 46,
    LS_SURF_BACK     = 47,
    LS_SURF_LEFT     = 48,
    LS_SURF_RIGHT    = 49,
    LS_USE_MIDAS     = 50,
    LS_DIE_MIDAS     = 51,
    LS_SWAN_DIVE     = 52,
    LS_FAST_DIVE     = 53,
    LS_GYMNAST       = 54,
    LS_WATER_OUT     = 55,
#if TR_VERSION      >= 2
    LS_CLIMB_STANCE  = 56,
    LS_CLIMBING      = 57,
    LS_CLIMB_LEFT    = 58,
    LS_CLIMB_END     = 59,
    LS_CLIMB_RIGHT   = 60,
    LS_CLIMB_DOWN    = 61,
    LS_LARA_TEST1    = 62,
    LS_LARA_TEST2    = 63,
    LS_LARA_TEST3    = 64,
    LS_WADE          = 65,
    LS_WATER_ROLL    = 66,
    LS_FLARE_PICKUP  = 67,
    LS_TWIST         = 68,
    LS_KICK          = 69,
    LS_ZIPLINE       = 70,
#elif TR_VERSION    == 1
    LS_CONTROLLED    = 56,
    LS_TWIST         = 57,
    LS_UW_ROLL       = 58,
    LS_WADE          = 59,
    LS_RESPONSIVE    = 60,
#endif
} LARA_STATE;
// clang-format on

#if TR_VERSION == 1
    #include "enum_tr1.h"
#elif TR_VERSION == 2
    #include "enum_tr2.h"
#endif
