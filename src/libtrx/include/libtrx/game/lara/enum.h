#pragma once

typedef enum {
    LS_INVALID = -1,
} LARA_STATE;

typedef enum {
    LS_TRX_INVALID = -1,
#define X_CATALOG_ID(enum_value) enum_value,
#include "../catalog_lara_states.def"
#undef X_CATALOG_ID
    LS_NUMBER_OF,
} LARA_TRX_STATE;

typedef enum {
    LA_INVALID = -1,
} LARA_ANIMATION;

typedef enum {
    LA_TRX_INVALID = -1,
#define X_CATALOG_ID(enum_value) enum_value,
#include "../catalog_lara_anims.def"
#undef X_CATALOG_ID
    LA_NUMBER_OF,
} LARA_TRX_ANIMATION;

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
// clang-format on

// clang-format off
typedef enum {
    LS_EXTRA_BREATH      = 0,
#if TR_VERSION == 1
    // TODO: refactor extra animations. Use catalog?
    LS_EXTRA_PLUNGER     = -1,
    LS_EXTRA_GONG_BONG   = -1,
    LS_EXTRA_YETI_KILL   = -1,
    LS_EXTRA_PULL_DAGGER = -1,
    LS_EXTRA_SHARK_KILL  = -1,
    LS_EXTRA_FINAL_ANIM  = -1,
    LS_EXTRA_TREX_KILL   = 1,
#elif TR_VERSION == 2
    LS_EXTRA_PLUNGER     = 1,
    LS_EXTRA_YETI_KILL   = 2,
    LS_EXTRA_SHARK_KILL  = 3,
    LS_EXTRA_AIRLOCK     = 4,
    LS_EXTRA_GONG_BONG   = 5,
    LS_EXTRA_TREX_KILL   = 6,
    LS_EXTRA_PULL_DAGGER = 7,
    LS_EXTRA_START_ANIM  = 8,
    LS_EXTRA_START_HOUSE = 9,
    LS_EXTRA_FINAL_ANIM  = 10,
#endif
    LS_EXTRA_NUMBER_OF,
} LARA_EXTRA_STATE;
// clang-format on

// clang-format off
typedef enum {
    LGT_UNKNOWN = -1, // for legacy saves
    LGT_UNARMED = 0,
    LGT_PISTOLS = 1,
    LGT_MAGNUMS = 2,
    LGT_UZIS    = 3,
    LGT_SHOTGUN = 4,
    LGT_M16     = 5,
    LGT_GRENADE = 6,
    LGT_HARPOON = 7,
    LGT_FLARE   = 8,
    LGT_SKIDOO  = 9,
    NUM_WEAPONS,
} LARA_GUN_TYPE;
// clang-format on

#if TR_VERSION == 1
    #include "enum_tr1.h"
#elif TR_VERSION == 2
    #include "enum_tr2.h"
#endif
