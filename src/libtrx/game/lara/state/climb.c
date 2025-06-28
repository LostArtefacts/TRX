#include "config.h"
#include "game/camera.h"
#include "game/input.h"
#include "game/lara.h"
#include "game/lara/util.h"

// clang-format off
#define M_CAM_CLIMB_LEFT_ANGLE       (-30 * DEG_1)              // = -5460
#define M_CAM_CLIMB_LEFT_ELEVATION   (-15 * DEG_1)              // = -2730
#define M_CAM_CLIMB_RIGHT_ANGLE      (-M_CAM_CLIMB_LEFT_ANGLE)  // = 5460
#define M_CAM_CLIMB_RIGHT_ELEVATION  M_CAM_CLIMB_LEFT_ELEVATION // = -2730
#define M_CAM_CLIMB_STANCE_ELEVATION (-20 * DEG_1)              // = -3640
#define M_CAM_CLIMBING_ELEVATION     (30 * DEG_1)               // = 5460
#define M_CAM_CLIMB_END_ELEVATION    (-45 * DEG_1)              // = -8190
#define M_CAM_CLIMB_DOWN_ELEVATION   M_CAM_CLIMB_END_ELEVATION  // = -8190
// clang-format on

static void M_Hang(ITEM *item, COLL_INFO *coll);
static void M_StanceLadder(ITEM *item, COLL_INFO *coll);
static void M_SideLadder(ITEM *item, COLL_INFO *coll);
static void M_UpDownLadder(ITEM *item, COLL_INFO *coll);

static void M_Hang(ITEM *const item, COLL_INFO *const coll)
{
#if TR_VERSION == 1
    const bool look = g_Config.gameplay.enable_enhanced_look && g_Input.look;
#else
    const bool look = g_Input.look;
#endif
    if (look) {
        Lara_LookUpDown();
    }

    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_HANG_ANGLE;
    g_Camera.target_elevation = CAM_HANG_ELEVATION;
    if (g_Input.left || g_Input.step_left) {
        item->goal_anim_state = LS_SHIMMY_LEFT;
    } else if (g_Input.right || g_Input.step_right) {
        item->goal_anim_state = LS_SHIMMY_RIGHT;
    }
}

static void M_StanceLadder(ITEM *const item, COLL_INFO *const coll)
{
#if TR_VERSION >= 2
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_elevation = M_CAM_CLIMB_STANCE_ELEVATION;

    if (g_Input.look) {
        Lara_LookUpDown();
    }

    if (g_Input.left || g_Input.step_left) {
        item->goal_anim_state = LS_CLIMB_LEFT;
    } else if (g_Input.right || g_Input.step_right) {
        item->goal_anim_state = LS_CLIMB_RIGHT;
    } else if (g_Input.jump) {
        item->goal_anim_state = LS_JUMP_BACK;
        LARA_INFO *const lara = Lara_GetLaraInfo();
        lara->gun_status = LGS_ARMLESS;
        lara->move_angle = item->rot.y + DEG_180;
    }
#endif
}

static void M_SideLadder(ITEM *const item, COLL_INFO *const coll)
{
#if TR_VERSION >= 2
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    if (item->current_anim_state == LS_CLIMB_LEFT) {
        g_Camera.target_angle = M_CAM_CLIMB_LEFT_ANGLE;
        g_Camera.target_elevation = M_CAM_CLIMB_LEFT_ELEVATION;
    } else {
        g_Camera.target_angle = M_CAM_CLIMB_RIGHT_ANGLE;
        g_Camera.target_elevation = M_CAM_CLIMB_RIGHT_ELEVATION;
    }
    if (!g_Input.right && !g_Input.step_right) {
        item->goal_anim_state = LS_CLIMB_STANCE;
    }
#endif
}

static void M_UpDownLadder(ITEM *const item, COLL_INFO *const coll)
{
#if TR_VERSION >= 2
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    switch (item->current_anim_state) {
    case LS_CLIMBING:
        g_Camera.target_elevation = M_CAM_CLIMBING_ELEVATION;
        break;
    case LS_CLIMB_DOWN:
        g_Camera.target_elevation = M_CAM_CLIMB_DOWN_ELEVATION;
        break;
    case LS_CLIMB_END:
        g_Camera.flags = CF_FOLLOW_CENTRE;
        g_Camera.target_angle = M_CAM_CLIMB_END_ELEVATION;
        break;
    default:
        break;
    }
#endif
}

// clang-format off
REGISTER_LARA_STATE(LS_HANG,         M_Hang)
#if TR_VERSION >= 2
REGISTER_LARA_STATE(LS_CLIMB_STANCE, M_StanceLadder)
REGISTER_LARA_STATE(LS_CLIMB_LEFT,   M_SideLadder)
REGISTER_LARA_STATE(LS_CLIMB_RIGHT,  M_SideLadder)
REGISTER_LARA_STATE(LS_CLIMBING,     M_UpDownLadder)
REGISTER_LARA_STATE(LS_CLIMB_DOWN,   M_UpDownLadder)
REGISTER_LARA_STATE(LS_CLIMB_END,    M_UpDownLadder)
#endif
// clang-format on
