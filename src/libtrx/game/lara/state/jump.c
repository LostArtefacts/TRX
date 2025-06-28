#include "config.h"
#include "game/camera.h"
#include "game/input.h"
#include "game/lara.h"
#include "game/lara/util.h"

#define M_CAM_BACK_JUMP_ANGLE (135 * DEG_1) // = 24570

static bool IsJumpTwistEnabled(void);

static void M_Compress(ITEM *item, COLL_INFO *coll);
static void M_UpJump(ITEM *item, COLL_INFO *coll);
static void M_ForwardJump(ITEM *item, COLL_INFO *coll);
static void M_BackJump(ITEM *item, COLL_INFO *coll);
static void M_SideJump(ITEM *item, COLL_INFO *coll);

static bool IsJumpTwistEnabled(void)
{
#if TR_VERSION == 1
    return g_Config.gameplay.enable_jump_twists;
#else
    return true;
#endif
}

static void M_Compress(ITEM *const item, COLL_INFO *const coll)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->water_status != LWS_WADE) {
        if (g_Input.forward
            && Lara_FloorFront(item, item->rot.y, STEP_L) >= -STEPUP_HEIGHT) {
            item->goal_anim_state = LS_JUMP_FORWARD;
            lara->move_angle = item->rot.y;
        } else if (
            g_Input.left
            && Lara_FloorFront(item, item->rot.y - DEG_90, STEP_L)
                >= -STEPUP_HEIGHT) {
            item->goal_anim_state = LS_JUMP_LEFT;
            lara->move_angle = item->rot.y - DEG_90;
        } else if (
            g_Input.right
            && Lara_FloorFront(item, item->rot.y + DEG_90, STEP_L)
                >= -STEPUP_HEIGHT) {
            item->goal_anim_state = LS_JUMP_RIGHT;
            lara->move_angle = item->rot.y + DEG_90;
        } else if (
            g_Input.back
            && Lara_FloorFront(item, item->rot.y + DEG_180, STEP_L)
                >= -STEPUP_HEIGHT) {
            item->goal_anim_state = LS_JUMP_BACK;
            lara->move_angle = item->rot.y + DEG_180;
        }
    }

    if (item->fall_speed > LARA_FAST_FALL_SPEED) {
        item->goal_anim_state = LS_FAST_FALL;
    }
}

static void M_UpJump(ITEM *item, COLL_INFO *coll)
{
    const int16_t fast_speed = g_Config.gameplay.enable_swing_cancel
        ? LARA_SWING_FAST_FALL_SPEED
        : LARA_FAST_FALL_SPEED;
    if (item->fall_speed > fast_speed) {
        item->goal_anim_state = LS_FAST_FALL;
    }
}

static void M_ForwardJump(ITEM *const item, COLL_INFO *const coll)
{
    if (item->goal_anim_state == LS_SWAN_DIVE
        || item->goal_anim_state == LS_REACH) {
        item->goal_anim_state = LS_JUMP_FORWARD;
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (item->goal_anim_state != LS_DEATH && item->goal_anim_state != LS_STOP
        && item->goal_anim_state != LS_RUN) {
        if (g_Input.action && lara->gun_status == LGS_ARMLESS) {
            item->goal_anim_state = LS_REACH;
        }
        if (IsJumpTwistEnabled() && (g_Input.roll || g_Input.back)) {
            item->goal_anim_state = LS_TWIST;
        }
        if (g_Input.slow && lara->gun_status == LGS_ARMLESS) {
            item->goal_anim_state = LS_SWAN_DIVE;
        }
        if (item->fall_speed > LARA_FAST_FALL_SPEED) {
            item->goal_anim_state = LS_FAST_FALL;
        }
    }

    if (g_Input.left) {
        lara->turn_rate -= LARA_TURN_RATE;
        CLAMPL(lara->turn_rate, -LARA_JUMP_TURN);
    } else if (g_Input.right) {
        lara->turn_rate += LARA_TURN_RATE;
        CLAMPG(lara->turn_rate, +LARA_JUMP_TURN);
    }
}

static void M_BackJump(ITEM *const item, COLL_INFO *const coll)
{
    g_Camera.target_angle = M_CAM_BACK_JUMP_ANGLE;
    if (item->fall_speed > LARA_FAST_FALL_SPEED) {
        item->goal_anim_state = LS_FAST_FALL;
        return;
    }

    if (item->goal_anim_state == LS_RUN) {
        item->goal_anim_state = LS_STOP;
    } else if (
        IsJumpTwistEnabled() && (g_Input.forward || g_Input.roll)
        && item->goal_anim_state != LS_STOP) {
        item->goal_anim_state = LS_TWIST;
    }
}

static void M_SideJump(ITEM *const item, COLL_INFO *const coll)
{
#if TR_VERSION >= 2
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->enable_look = false;
#endif
    if (item->fall_speed > LARA_FAST_FALL_SPEED) {
        item->goal_anim_state = LS_FAST_FALL;
        return;
    }

    // TODO: unused animation transition, perhaps look at restoring
    const bool twist_input =
        item->current_anim_state == LS_JUMP_LEFT ? g_Input.right : g_Input.left;
    if (IsJumpTwistEnabled() && twist_input
        && item->goal_anim_state != LS_STOP) {
        item->goal_anim_state = LS_TWIST;
    }
}

// clang-format off
REGISTER_LARA_STATE(LS_COMPRESS,     M_Compress)
REGISTER_LARA_STATE(LS_JUMP_UP,      M_UpJump)
REGISTER_LARA_STATE(LS_JUMP_FORWARD, M_ForwardJump)
REGISTER_LARA_STATE(LS_JUMP_BACK,    M_BackJump)
REGISTER_LARA_STATE(LS_JUMP_RIGHT,   M_SideJump)
REGISTER_LARA_STATE(LS_JUMP_LEFT,    M_SideJump)
// clang-format on
