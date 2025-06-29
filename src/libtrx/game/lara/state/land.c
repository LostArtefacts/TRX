#include "config.h"
#include "game/camera.h"
#include "game/input.h"
#include "game/lara.h"
#include "game/lara/util.h"

#define M_LF_ROLL 2
#define M_LF_FLARE_PICKUP_END 89
#define M_LF_UW_FLARE_PICKUP_END 35
#if TR_VERSION == 1
    #define M_LF_JUMP_READY 2
#else
    #define M_LF_JUMP_READY 4
#endif

#define M_CAM_SLIDE_ELEVATION (-45 * DEG_1) // = -8190
#define M_CAM_PUSH_BLOCK_ANGLE (35 * DEG_1) // = 6370
#define M_CAM_PUSH_BLOCK_ELEVATION (-25 * DEG_1) // = -4550
#define M_CAM_PP_READY_ANGLE (75 * DEG_1) // = 13650
#define M_CAM_PICKUP_ANGLE (-130 * DEG_1) // = -23660
#define M_CAM_PICKUP_ELEVATION (-15 * DEG_1) // = -2730
#define M_CAM_PICKUP_DISTANCE WALL_L // = 1024
#define M_CAM_SWITCH_ON_ANGLE (80 * DEG_1) // = 14560
#define M_CAM_SWITCH_ON_ELEVATION (-25 * DEG_1) // = -4550
#define M_CAM_SWITCH_ON_DISTANCE WALL_L // = 1024
#define M_CAM_SWITCH_ON_SPEED 6
#define M_CAM_USE_KEY_ANGLE (-M_CAM_SWITCH_ON_ANGLE) // = -14560
#define M_CAM_USE_KEY_ELEVATION M_CAM_SWITCH_ON_ELEVATION // = -4550
#define M_CAM_USE_KEY_DISTANCE WALL_L // = 1024

static bool m_JumpPermitted = true;

static void M_Default(ITEM *item, COLL_INFO *coll);
static void M_Walk(ITEM *item, COLL_INFO *coll);
static void M_Run(ITEM *item, COLL_INFO *coll);
static void M_Stop(ITEM *item, COLL_INFO *coll);
static void M_FastBack(ITEM *item, COLL_INFO *coll);
static void M_Turn(ITEM *item, COLL_INFO *coll);
static void M_FastTurn(ITEM *item, COLL_INFO *coll);
static void M_Death(ITEM *item, COLL_INFO *coll);
static void M_Splat(ITEM *item, COLL_INFO *coll);
static void M_WalkBack(ITEM *item, COLL_INFO *coll);
static void M_SideStep(ITEM *item, COLL_INFO *coll);
static void M_Slide(ITEM *item, COLL_INFO *coll);
static void M_PushBlock(ITEM *item, COLL_INFO *coll);
static void M_PPReady(ITEM *item, COLL_INFO *coll);
static void M_Pickup(ITEM *item, COLL_INFO *coll);
static void M_PickupFlare(ITEM *item, COLL_INFO *coll);
static void M_SwitchOn(ITEM *item, COLL_INFO *coll);
static void M_UseKey(ITEM *item, COLL_INFO *coll);
static void M_Wade(ITEM *item, COLL_INFO *coll);

static void M_Default(ITEM *const item, COLL_INFO *const coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
}

static void M_Walk(ITEM *const item, COLL_INFO *const coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_STOP;
        return;
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (g_Input.left) {
        lara->turn_rate -= LARA_TURN_RATE;
        CLAMPL(lara->turn_rate, -LARA_SLOW_TURN);
    } else if (g_Input.right) {
        lara->turn_rate += LARA_TURN_RATE;
        CLAMPG(lara->turn_rate, +LARA_SLOW_TURN);
    }

    if (g_Input.forward) {
        if (lara->water_status == LWS_WADE) {
            item->goal_anim_state = LS_WADE;
        } else if (g_Input.slow) {
            item->goal_anim_state = LS_WALK;
        } else {
#if TR_VERSION == 1
            const bool fix_walk_run_jump = true;
#else
            const bool fix_walk_run_jump = g_Config.gameplay.fix_walk_run_jump;
#endif
            if (fix_walk_run_jump) {
                m_JumpPermitted = true;
            }
            item->goal_anim_state = LS_RUN;
        }
    } else {
        item->goal_anim_state = LS_STOP;
    }
}

static void M_Run(ITEM *const item, COLL_INFO *const coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_DEATH;
        return;
    }

    if (g_Input.roll) {
        item->current_anim_state = LS_ROLL;
        item->goal_anim_state = LS_STOP;
        Item_SwitchToAnim(item, LA_ROLL_START, M_LF_ROLL);
        return;
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (g_Input.left) {
        lara->turn_rate -= LARA_TURN_RATE;
        CLAMPL(lara->turn_rate, -LARA_FAST_TURN);
        item->rot.z -= LARA_LEAN_RATE;
        CLAMPL(item->rot.z, -LARA_LEAN_MAX);
    } else if (g_Input.right) {
        lara->turn_rate += LARA_TURN_RATE;
        CLAMPG(lara->turn_rate, +LARA_FAST_TURN);
        item->rot.z += LARA_LEAN_RATE;
        CLAMPG(item->rot.z, +LARA_LEAN_MAX);
    }

#if TR_VERSION == 1
    const bool responsive_jumping =
        g_Config.gameplay.enable_tr2_jumping && Lara_State_IsResponsive(LA_RUN);
#else
    const bool responsive_jumping = true;
#endif
    if (responsive_jumping) {
        if (Item_TestAnimEqual(item, LA_RUN_START)) {
            m_JumpPermitted = false;
        } else if (
            !Item_TestAnimEqual(item, LA_RUN)
            || Item_TestFrameEqual(item, M_LF_JUMP_READY)) {
            m_JumpPermitted = true;
        }
    } else {
        m_JumpPermitted = true;
    }

    if (g_Input.jump && m_JumpPermitted && !item->gravity) {
#if TR_VERSION == 1
        item->goal_anim_state =
            responsive_jumping ? LS_RESPONSIVE : LS_JUMP_FORWARD;
#else
        item->goal_anim_state = LS_JUMP_FORWARD;
#endif
    } else if (g_Input.forward) {
        if (lara->water_status == LWS_WADE) {
            item->goal_anim_state = LS_WADE;
        } else if (g_Input.slow) {
            item->goal_anim_state = LS_WALK;
        } else {
            item->goal_anim_state = LS_RUN;
        }
    } else {
        item->goal_anim_state = LS_STOP;
    }
}

static void M_Stop(ITEM *const item, COLL_INFO *const coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_DEATH;
        return;
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
#if TR_VERSION == 1
    if (lara->interact_target.is_moving) {
        return;
    }

    const bool enable_enhanced_look = g_Config.gameplay.enable_enhanced_look;
#else
    const bool enable_enhanced_look = true;
#endif

    if (g_Input.roll && lara->water_status != LWS_WADE) {
        item->current_anim_state = LS_ROLL;
        item->goal_anim_state = LS_STOP;
        Item_SwitchToAnim(item, LA_ROLL_START, M_LF_ROLL);
        return;
    }

    item->goal_anim_state = LS_STOP;
    if (g_Input.look) {
        Lara_LookUpDown();
        if (!enable_enhanced_look) {
            Lara_LookLeftRight();
            return;
        }
    }

    if (!enable_enhanced_look && g_Camera.type == CAM_LOOK) {
        g_Camera.type = CAM_CHASE;
    }

    if (g_Input.step_left) {
        item->goal_anim_state = LS_STEP_LEFT;
    } else if (g_Input.step_right) {
        item->goal_anim_state = LS_STEP_RIGHT;
    } else if (g_Input.left) {
        item->goal_anim_state = LS_TURN_LEFT;
    } else if (g_Input.right) {
        item->goal_anim_state = LS_TURN_RIGHT;
    }

    if (lara->water_status == LWS_WADE) {
        if (g_Input.jump) {
            item->goal_anim_state = LS_COMPRESS;
        }

        if (g_Input.forward) {
            if (g_Input.slow) {
                M_Wade(item, coll);
            } else {
                M_Walk(item, coll);
            }
        } else if (g_Input.back) {
            M_WalkBack(item, coll);
        }
    } else if (g_Input.jump) {
        item->goal_anim_state = LS_COMPRESS;
    } else if (g_Input.forward) {
        if (g_Input.slow) {
            M_Walk(item, coll);
        } else {
            M_Run(item, coll);
        }
    } else if (g_Input.back) {
        if (g_Input.slow) {
            M_WalkBack(item, coll);
        } else {
            item->goal_anim_state = LS_FAST_BACK;
        }
    }
}

static void M_FastBack(ITEM *const item, COLL_INFO *const coll)
{
    item->goal_anim_state = LS_STOP;
    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (g_Input.left) {
        lara->turn_rate -= LARA_TURN_RATE;
        CLAMPL(lara->turn_rate, -LARA_MED_TURN);
    } else if (g_Input.right) {
        lara->turn_rate += LARA_TURN_RATE;
        CLAMPG(lara->turn_rate, LARA_MED_TURN);
    }
}

static void M_Turn(ITEM *const item, COLL_INFO *const coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_STOP;
        return;
    }

#if TR_VERSION == 1
    if (g_Config.gameplay.enable_enhanced_look && g_Input.look) {
        item->goal_anim_state = LS_STOP;
        return;
    }
#endif

    const bool left_turn = item->current_anim_state == LS_TURN_LEFT;
    const bool turn_input = left_turn ? g_Input.left : g_Input.right;

    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (left_turn) {
        lara->turn_rate -= LARA_TURN_RATE;
    } else {
        lara->turn_rate += LARA_TURN_RATE;
    }

    if (lara->gun_status == LGS_READY) {
        item->goal_anim_state = LS_FAST_TURN;
    } else if (left_turn && lara->turn_rate < -LARA_SLOW_TURN) {
        if (g_Input.slow) {
            lara->turn_rate = -LARA_SLOW_TURN;
        } else {
            item->goal_anim_state = LS_FAST_TURN;
        }
    } else if (!left_turn && lara->turn_rate > LARA_SLOW_TURN) {
        if (g_Input.slow) {
            lara->turn_rate = LARA_SLOW_TURN;
        } else {
            item->goal_anim_state = LS_FAST_TURN;
        }
    }

    if (g_Input.forward) {
        if (lara->water_status == LWS_WADE) {
            item->goal_anim_state = LS_WADE;
        } else if (g_Input.slow) {
            item->goal_anim_state = LS_WALK;
        } else {
            item->goal_anim_state = LS_RUN;
        }
    } else if (!turn_input) {
        item->goal_anim_state = LS_STOP;
    }
}

static void M_FastTurn(ITEM *const item, COLL_INFO *const coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_STOP;
        return;
    }

#if TR_VERSION == 1
    if (g_Config.gameplay.enable_enhanced_look && g_Input.look) {
        item->goal_anim_state = LS_STOP;
        return;
    }
#endif

    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->turn_rate >= 0) {
        lara->turn_rate = LARA_FAST_TURN;
        if (!g_Input.right) {
            item->goal_anim_state = LS_STOP;
        }
    } else {
        lara->turn_rate = -LARA_FAST_TURN;
        if (!g_Input.left) {
            item->goal_anim_state = LS_STOP;
        }
    }
}

static void M_Death(ITEM *item, COLL_INFO *coll)
{
#if TR_VERSION >= 2
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->enable_look = false;
#endif
    M_Default(item, coll);
}

static void M_Splat(ITEM *const item, COLL_INFO *const coll)
{
#if TR_VERSION >= 2
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->enable_look = false;
#endif
}

static void M_WalkBack(ITEM *const item, COLL_INFO *const coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_STOP;
        return;
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (g_Input.back && (g_Input.slow || lara->water_status == LWS_WADE)) {
        item->goal_anim_state = LS_WALK_BACK;
    } else {
        item->goal_anim_state = LS_STOP;
    }

    if (g_Input.left) {
        lara->turn_rate -= LARA_TURN_RATE;
        CLAMPL(lara->turn_rate, -LARA_SLOW_TURN);
    } else if (g_Input.right) {
        lara->turn_rate += LARA_TURN_RATE;
        CLAMPG(lara->turn_rate, LARA_SLOW_TURN);
    }
}

static void M_SideStep(ITEM *const item, COLL_INFO *const coll)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
#if TR_VERSION >= 2
    lara->enable_look = false;
#endif
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_STOP;
        return;
    }

    const bool step_input = item->current_anim_state == LS_STEP_LEFT
        ? g_Input.step_left
        : g_Input.step_right;
    if (!step_input) {
        item->goal_anim_state = LS_STOP;
    }

    if (g_Input.left) {
        lara->turn_rate -= LARA_TURN_RATE;
        CLAMPL(lara->turn_rate, -LARA_SLOW_TURN);
    } else if (g_Input.right) {
        lara->turn_rate += LARA_TURN_RATE;
        CLAMPG(lara->turn_rate, LARA_SLOW_TURN);
    }
}

static void M_Slide(ITEM *const item, COLL_INFO *const coll)
{
#if TR_VERSION == 1
    const bool enable_jump_twists = g_Config.gameplay.enable_jump_twists;
#else
    const bool enable_jump_twists = true;
#endif
    bool opposite_input;
    if (item->current_anim_state == LS_SLIDE) {
        g_Camera.flags = CF_NO_CHUNKY;
        g_Camera.target_elevation = M_CAM_SLIDE_ELEVATION;
        opposite_input = g_Input.back;
    } else {
        opposite_input = g_Input.forward;
    }

    if (g_Input.jump && (!enable_jump_twists || !opposite_input)) {
        item->goal_anim_state = item->current_anim_state == LS_SLIDE
            ? LS_JUMP_FORWARD
            : LS_JUMP_BACK;
    }
}

static void M_PushBlock(ITEM *const item, COLL_INFO *const coll)
{
#if TR_VERSION >= 2
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->enable_look = false;
#endif
    M_Default(item, coll);
    g_Camera.flags = CF_FOLLOW_CENTRE;
    g_Camera.target_angle = M_CAM_PUSH_BLOCK_ANGLE;
    g_Camera.target_elevation = M_CAM_PUSH_BLOCK_ELEVATION;
}

static void M_PPReady(ITEM *const item, COLL_INFO *const coll)
{
    M_Default(item, coll);
    g_Camera.target_angle = M_CAM_PP_READY_ANGLE;
    if (!g_Input.action) {
        item->goal_anim_state = LS_STOP;
    }
}

static void M_Pickup(ITEM *const item, COLL_INFO *const coll)
{
#if TR_VERSION >= 2
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->enable_look = false;
#endif
    M_Default(item, coll);
    g_Camera.target_angle = M_CAM_PICKUP_ANGLE;
    g_Camera.target_elevation = M_CAM_PICKUP_ELEVATION;
    g_Camera.target_distance = M_CAM_PICKUP_DISTANCE;
}

static void M_PickupFlare(ITEM *const item, COLL_INFO *const coll)
{
#if TR_VERSION >= 2
    M_Pickup(item, coll);
    const int16_t frame_num = Item_TestAnimEqual(item, LA_FLARE_PICKUP)
        ? M_LF_FLARE_PICKUP_END
        : M_LF_UW_FLARE_PICKUP_END;
    if (Item_TestFrameEqual(item, frame_num)) {
        LARA_INFO *const lara = Lara_GetLaraInfo();
        lara->gun_status = LGS_ARMLESS;
    }
#endif
}

static void M_SwitchOn(ITEM *const item, COLL_INFO *const coll)
{
#if TR_VERSION >= 2
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->enable_look = false;
#endif
    M_Default(item, coll);
    g_Camera.target_angle = M_CAM_SWITCH_ON_ANGLE;
    g_Camera.target_elevation = M_CAM_SWITCH_ON_ELEVATION;
    g_Camera.target_distance = M_CAM_SWITCH_ON_DISTANCE;
    g_Camera.speed = M_CAM_SWITCH_ON_SPEED;
}

static void M_UseKey(ITEM *const item, COLL_INFO *const coll)
{
#if TR_VERSION >= 2
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->enable_look = false;
#endif
    M_Default(item, coll);
    g_Camera.target_angle = M_CAM_USE_KEY_ANGLE;
    g_Camera.target_elevation = M_CAM_USE_KEY_ELEVATION;
    g_Camera.target_distance = M_CAM_USE_KEY_DISTANCE;
}

static void M_Wade(ITEM *const item, COLL_INFO *const coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_STOP;
        return;
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    g_Camera.target_elevation = CAM_WADE_ELEVATION;
    if (g_Input.left) {
        lara->turn_rate -= LARA_TURN_RATE;
        CLAMPL(lara->turn_rate, -LARA_FAST_TURN);
        item->rot.z -= LARA_LEAN_RATE;
        CLAMPL(item->rot.z, -LARA_LEAN_MAX);
    } else if (g_Input.right) {
        lara->turn_rate += LARA_TURN_RATE;
        CLAMPG(lara->turn_rate, LARA_FAST_TURN);
        item->rot.z += LARA_LEAN_RATE;
        CLAMPG(item->rot.z, LARA_LEAN_MAX);
    }

    if (g_Input.forward) {
        if (lara->water_status != LWS_ABOVE_WATER) {
            item->goal_anim_state = LS_WADE;
        } else {
            item->goal_anim_state = LS_RUN;
        }
    } else {
        item->goal_anim_state = LS_STOP;
    }
}

// clang-format off
REGISTER_LARA_STATE(LS_PULL_UP,      M_Default)
REGISTER_LARA_STATE(LS_GYMNAST,      M_Default)
REGISTER_LARA_STATE(LS_WALK,         M_Walk)
REGISTER_LARA_STATE(LS_RUN,          M_Run)
REGISTER_LARA_STATE(LS_STOP,         M_Stop)
REGISTER_LARA_STATE(LS_FAST_BACK,    M_FastBack)
REGISTER_LARA_STATE(LS_TURN_RIGHT,   M_Turn)
REGISTER_LARA_STATE(LS_TURN_LEFT,    M_Turn)
REGISTER_LARA_STATE(LS_FAST_TURN,    M_FastTurn)
REGISTER_LARA_STATE(LS_DEATH,        M_Death)
REGISTER_LARA_STATE(LS_SPLAT,        M_Splat)
REGISTER_LARA_STATE(LS_WALK_BACK,    M_WalkBack)
REGISTER_LARA_STATE(LS_STEP_RIGHT,   M_SideStep)
REGISTER_LARA_STATE(LS_STEP_LEFT,    M_SideStep)
REGISTER_LARA_STATE(LS_SLIDE,        M_Slide)
REGISTER_LARA_STATE(LS_SLIDE_BACK,   M_Slide)
REGISTER_LARA_STATE(LS_PUSH_BLOCK,   M_PushBlock)
REGISTER_LARA_STATE(LS_PULL_BLOCK,   M_PushBlock)
REGISTER_LARA_STATE(LS_PP_READY,     M_PPReady)
REGISTER_LARA_STATE(LS_PICKUP,       M_Pickup)
REGISTER_LARA_STATE(LS_SWITCH_ON,    M_SwitchOn)
REGISTER_LARA_STATE(LS_SWITCH_OFF,   M_SwitchOn)
REGISTER_LARA_STATE(LS_USE_KEY,      M_UseKey)
REGISTER_LARA_STATE(LS_USE_PUZZLE,   M_UseKey)
REGISTER_LARA_STATE(LS_WADE,         M_Wade)
#if TR_VERSION == 1
REGISTER_LARA_STATE(LS_CONTROLLED,   M_Default)
#else
REGISTER_LARA_STATE(LS_FLARE_PICKUP, M_PickupFlare)
#endif
// clang-format on
