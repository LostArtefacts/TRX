#include "config.h"
#include "game/camera.h"
#include "game/input.h"
#include "game/lara.h"
#include "game/lara/util.h"

#define M_LF_ROLL 2
#if TR_VERSION == 1
    #define M_LF_JUMP_READY 2
#else
    #define M_LF_JUMP_READY 4
#endif

static bool m_JumpPermitted = true;

static void M_Default(ITEM *item, COLL_INFO *coll);
static void M_Walk(ITEM *item, COLL_INFO *coll);
static void M_Run(ITEM *item, COLL_INFO *coll);
static void M_Stop(ITEM *item, COLL_INFO *coll);
static void M_FastBack(ITEM *item, COLL_INFO *coll);
static void M_Turn(ITEM *item, COLL_INFO *coll);
static void M_Death(ITEM *item, COLL_INFO *coll);
static void M_Splat(ITEM *item, COLL_INFO *coll);
static void M_WalkBack(ITEM *item, COLL_INFO *coll);
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
REGISTER_LARA_STATE(LS_DEATH,        M_Death)
REGISTER_LARA_STATE(LS_SPLAT,        M_Splat)
REGISTER_LARA_STATE(LS_WALK_BACK,    M_WalkBack)
REGISTER_LARA_STATE(LS_WADE,         M_Wade)
#if TR_VERSION == 1
REGISTER_LARA_STATE(LS_CONTROLLED,   M_Default)
#endif
// clang-format on
