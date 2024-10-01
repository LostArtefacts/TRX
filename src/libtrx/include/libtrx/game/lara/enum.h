#pragma once

// clang-format off
typedef enum {
    LWS_ABOVE_WATER  = 0,
    LWS_UNDERWATER   = 1,
    LWS_SURFACE      = 2,
    LWS_CHEAT        = 3,
#if TR_VERSION == 2
    LWS_WADE         = 4,
#endif
} LARA_WATER_STATE;
// clang-format on

// clang-format off
typedef enum {
    LGS_ARMLESS    = 0,
    LGS_HANDS_BUSY = 1,
    LGS_DRAW       = 2,
    LGS_UNDRAW     = 3,
    LGS_READY      = 4,
#if TR_VERSION == 2
    LGS_SPECIAL = 5,
#endif
} LARA_GUN_STATE;
// clang-format on

typedef enum {
    LM_HIPS = 0,
    LM_THIGH_L = 1,
    LM_CALF_L = 2,
    LM_FOOT_L = 3,
    LM_THIGH_R = 4,
    LM_CALF_R = 5,
    LM_FOOT_R = 6,
    LM_TORSO = 7,
    LM_UARM_R = 8,
    LM_LARM_R = 9,
    LM_HAND_R = 10,
    LM_UARM_L = 11,
    LM_LARM_L = 12,
    LM_HAND_L = 13,
    LM_HEAD = 14,
    LM_FIRST = LM_HIPS,
    LM_NUMBER_OF = 15,
} LARA_MESH;

#if TR_VERSION == 1
    #include "enum_tr1.h"
#elif TR_VERSION == 2
    #include "enum_tr2.h"
#endif
