#include "game/input.h"
#include "game/inventory.h"
#include "game/lara/control.h"
#include "game/lara/look.h"
#include "game/lara/misc.h"
#include "game/overlay.h"
#include "game/sound.h"
#include "game/viewport.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/game.h>
#include <libtrx/game/lara/const.h>
#include <libtrx/game/lara/util.h>
#include <libtrx/game/music.h>
#include <libtrx/utils.h>

#define LF_ROLL 2
#define LF_JUMP_READY 4
#define LF_FLARE_PICKUP_END 89
#define LF_UW_FLARE_PICKUP_END 35

static bool m_JumpPermitted = true;

static void M_SwimTurn(ITEM *item);

static void M_Walk(ITEM *item, COLL_INFO *coll);
static void M_Run(ITEM *item, COLL_INFO *coll);
static void M_Stop(ITEM *item, COLL_INFO *coll);
static void M_ForwardJump(ITEM *item, COLL_INFO *coll);
static void M_FastBack(ITEM *item, COLL_INFO *coll);
static void M_TurnRight(ITEM *item, COLL_INFO *coll);
static void M_TurnLeft(ITEM *item, COLL_INFO *coll);
static void M_Death(ITEM *item, COLL_INFO *coll);
static void M_FastFall(ITEM *item, COLL_INFO *coll);
static void M_Hang(ITEM *item, COLL_INFO *coll);
static void M_Reach(ITEM *item, COLL_INFO *coll);
static void M_Splat(ITEM *item, COLL_INFO *coll);
static void M_Compress(ITEM *item, COLL_INFO *coll);
static void M_Back(ITEM *item, COLL_INFO *coll);
static void M_Null(ITEM *item, COLL_INFO *coll);
static void M_FastTurn(ITEM *item, COLL_INFO *coll);
static void M_StepRight(ITEM *item, COLL_INFO *coll);
static void M_StepLeft(ITEM *item, COLL_INFO *coll);
static void M_Slide(ITEM *item, COLL_INFO *coll);
static void M_BackJump(ITEM *item, COLL_INFO *coll);
static void M_RightJump(ITEM *item, COLL_INFO *coll);
static void M_LeftJump(ITEM *item, COLL_INFO *coll);
static void M_UpJump(ITEM *item, COLL_INFO *coll);
static void M_FallBack(ITEM *item, COLL_INFO *coll);
static void M_HangLeft(ITEM *item, COLL_INFO *coll);
static void M_HangRight(ITEM *item, COLL_INFO *coll);
static void M_SlideBack(ITEM *item, COLL_INFO *coll);
static void M_PushBlock(ITEM *item, COLL_INFO *coll);
static void M_PPReady(ITEM *item, COLL_INFO *coll);
static void M_Pickup(ITEM *item, COLL_INFO *coll);
static void M_PickupFlare(ITEM *item, COLL_INFO *coll);
static void M_SwitchOn(ITEM *item, COLL_INFO *coll);
static void M_UseKey(ITEM *item, COLL_INFO *coll);
static void M_Special(ITEM *item, COLL_INFO *coll);
static void M_SwanDive(ITEM *item, COLL_INFO *coll);
static void M_FastDive(ITEM *item, COLL_INFO *coll);
static void M_Wade(ITEM *item, COLL_INFO *coll);
static void M_Zipline(ITEM *item, COLL_INFO *coll);
static void M_ClimbLeft(ITEM *item, COLL_INFO *coll);
static void M_ClimbRight(ITEM *item, COLL_INFO *coll);
static void M_ClimbStance(ITEM *item, COLL_INFO *coll);
static void M_Climbing(ITEM *item, COLL_INFO *coll);
static void M_ClimbEnd(ITEM *item, COLL_INFO *coll);
static void M_ClimbDown(ITEM *item, COLL_INFO *coll);
static void M_SurfBack(ITEM *item, COLL_INFO *coll);
static void M_SurfLeft(ITEM *item, COLL_INFO *coll);
static void M_SurfRight(ITEM *item, COLL_INFO *coll);
static void M_SurfTread(ITEM *item, COLL_INFO *coll);
static void M_Swim(ITEM *item, COLL_INFO *coll);
static void M_Glide(ITEM *item, COLL_INFO *coll);
static void M_Tread(ITEM *item, COLL_INFO *coll);
static void M_UWDeath(ITEM *item, COLL_INFO *coll);

static void M_SwimTurn(ITEM *const item)
{
    if (g_Input.forward) {
        item->rot.x -= LARA_TURN_RATE_UW;
    } else if (g_Input.back) {
        item->rot.x += LARA_TURN_RATE_UW;
    }

    if (g_Input.left) {
        g_Lara.turn_rate -= LARA_TURN_RATE;
        CLAMPL(g_Lara.turn_rate, -LARA_MED_TURN);
        item->rot.z -= LARA_LEAN_RATE_SWIM;
    } else if (g_Input.right) {
        g_Lara.turn_rate += LARA_TURN_RATE;
        CLAMPG(g_Lara.turn_rate, LARA_MED_TURN);
        item->rot.z += LARA_LEAN_RATE_SWIM;
    }
}

static void M_Walk(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_STOP;
        return;
    }

    if (g_Input.left) {
        g_Lara.turn_rate -= LARA_TURN_RATE;
        CLAMPL(g_Lara.turn_rate, -LARA_SLOW_TURN);
    } else if (g_Input.right) {
        g_Lara.turn_rate += LARA_TURN_RATE;
        CLAMPG(g_Lara.turn_rate, +LARA_SLOW_TURN);
    }

    if (g_Input.forward) {
        if (g_Lara.water_status == LWS_WADE) {
            item->goal_anim_state = LS_WADE;
        } else if (g_Input.slow) {
            item->goal_anim_state = LS_WALK;
        } else {
            if (g_Config.gameplay.fix_walk_run_jump) {
                m_JumpPermitted = true;
            }
            item->goal_anim_state = LS_RUN;
        }
    } else {
        item->goal_anim_state = LS_STOP;
    }
}

static void M_Run(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_DEATH;
        return;
    }

    if (g_Input.roll) {
        item->current_anim_state = LS_ROLL;
        item->goal_anim_state = LS_STOP;
        Item_SwitchToAnim(item, LA_ROLL_START, LF_ROLL);
        return;
    }

    if (g_Input.left) {
        g_Lara.turn_rate -= LARA_TURN_RATE;
        CLAMPL(g_Lara.turn_rate, -LARA_FAST_TURN);
        item->rot.z -= LARA_LEAN_RATE;
        CLAMPL(item->rot.z, -LARA_LEAN_MAX);
    } else if (g_Input.right) {
        g_Lara.turn_rate += LARA_TURN_RATE;
        CLAMPG(g_Lara.turn_rate, +LARA_FAST_TURN);
        item->rot.z += LARA_LEAN_RATE;
        CLAMPG(item->rot.z, +LARA_LEAN_MAX);
    }

    if (Item_TestAnimEqual(item, LA_RUN_START)) {
        m_JumpPermitted = false;
    } else if (
        !Item_TestAnimEqual(item, LA_RUN)
        || Item_TestFrameEqual(item, LF_JUMP_READY)) {
        m_JumpPermitted = true;
    }

    if (g_Input.jump && m_JumpPermitted && !item->gravity) {
        item->goal_anim_state = LS_JUMP_FORWARD;
    } else if (g_Input.forward) {
        if (g_Lara.water_status == LWS_WADE) {
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

static void M_Stop(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_DEATH;
        return;
    }

    if (g_Input.roll && g_Lara.water_status != LWS_WADE) {
        item->current_anim_state = LS_ROLL;
        item->goal_anim_state = LS_STOP;
        Item_SwitchToAnim(item, LA_ROLL_START, LF_ROLL);
        return;
    }

    item->goal_anim_state = LS_STOP;
    if (g_Input.look) {
        Lara_LookUpDown();
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

    if (g_Lara.water_status == LWS_WADE) {
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
            M_Back(item, coll);
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
            M_Back(item, coll);
        } else {
            item->goal_anim_state = LS_FAST_BACK;
        }
    }
}

static void M_ForwardJump(ITEM *item, COLL_INFO *coll)
{
    if (item->goal_anim_state == LS_SWAN_DIVE
        || item->goal_anim_state == LS_REACH) {
        item->goal_anim_state = LS_JUMP_FORWARD;
    }

    if (item->goal_anim_state != LS_DEATH && item->goal_anim_state != LS_STOP
        && item->goal_anim_state != LS_RUN) {
        if (g_Input.action && g_Lara.gun_status == LGS_ARMLESS) {
            item->goal_anim_state = LS_REACH;
        }
        if (g_Input.roll || g_Input.back) {
            item->goal_anim_state = LS_TWIST;
        }
        if (g_Input.slow && g_Lara.gun_status == LGS_ARMLESS) {
            item->goal_anim_state = LS_SWAN_DIVE;
        }
        if (item->fall_speed > LARA_FAST_FALL_SPEED) {
            item->goal_anim_state = LS_FAST_FALL;
        }
    }

    if (g_Input.left) {
        g_Lara.turn_rate -= LARA_TURN_RATE;
        CLAMPL(g_Lara.turn_rate, -LARA_JUMP_TURN);
    } else if (g_Input.right) {
        g_Lara.turn_rate += LARA_TURN_RATE;
        CLAMPG(g_Lara.turn_rate, +LARA_JUMP_TURN);
    }
}

static void M_FastBack(ITEM *item, COLL_INFO *coll)
{
    item->goal_anim_state = LS_STOP;
    if (g_Input.left) {
        g_Lara.turn_rate -= LARA_TURN_RATE;
        CLAMPL(g_Lara.turn_rate, -LARA_MED_TURN);
    } else if (g_Input.right) {
        g_Lara.turn_rate += LARA_TURN_RATE;
        CLAMPG(g_Lara.turn_rate, LARA_MED_TURN);
    }
}

static void M_TurnRight(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_STOP;
        return;
    }

    g_Lara.turn_rate += LARA_TURN_RATE;

    if (g_Lara.gun_status == LGS_READY) {
        item->goal_anim_state = LS_FAST_TURN;
    } else if (g_Lara.turn_rate > LARA_SLOW_TURN) {
        if (g_Input.slow) {
            g_Lara.turn_rate = LARA_SLOW_TURN;
        } else {
            item->goal_anim_state = LS_FAST_TURN;
        }
    }

    if (g_Input.forward) {
        if (g_Lara.water_status == LWS_WADE) {
            item->goal_anim_state = LS_WADE;
        } else if (g_Input.slow) {
            item->goal_anim_state = LS_WALK;
        } else {
            item->goal_anim_state = LS_RUN;
        }
    } else if (!g_Input.right) {
        item->goal_anim_state = LS_STOP;
    }
}

static void M_TurnLeft(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_STOP;
        return;
    }

    g_Lara.turn_rate -= LARA_TURN_RATE;

    if (g_Lara.gun_status == LGS_READY) {
        item->goal_anim_state = LS_FAST_TURN;
    } else if (g_Lara.turn_rate < -LARA_SLOW_TURN) {
        if (g_Input.slow) {
            g_Lara.turn_rate = -LARA_SLOW_TURN;
        } else {
            item->goal_anim_state = LS_FAST_TURN;
        }
    }

    if (g_Input.forward) {
        if (g_Lara.water_status == LWS_WADE) {
            item->goal_anim_state = LS_WADE;
        } else if (g_Input.slow) {
            item->goal_anim_state = LS_WALK;
        } else {
            item->goal_anim_state = LS_RUN;
        }
    } else if (!g_Input.left) {
        item->goal_anim_state = LS_STOP;
    }
}

static void M_Death(ITEM *item, COLL_INFO *coll)
{
    g_Lara.enable_look = false;
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
}

static void M_FastFall(ITEM *item, COLL_INFO *coll)
{
    item->speed = item->speed * 95 / 100;
    if (item->fall_speed == DAMAGE_START + DAMAGE_LENGTH) {
        Sound_Effect(SFX_LARA_FALL, &item->pos, SPM_NORMAL);
    }
}

static void M_Hang(ITEM *item, COLL_INFO *coll)
{
    if (g_Input.look) {
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

static void M_Reach(ITEM *item, COLL_INFO *coll)
{
    g_Camera.target_angle = CAM_REACH_ANGLE;
    if (item->fall_speed > LARA_FAST_FALL_SPEED) {
        item->goal_anim_state = LS_FAST_FALL;
    }
}

static void M_Splat(ITEM *item, COLL_INFO *coll)
{
    g_Lara.enable_look = false;
}

static void M_Compress(ITEM *item, COLL_INFO *coll)
{
    if (g_Lara.water_status != LWS_WADE) {
        if (g_Input.forward
            && Lara_FloorFront(item, item->rot.y, STEP_L) >= -STEPUP_HEIGHT) {
            item->goal_anim_state = LS_JUMP_FORWARD;
            g_Lara.move_angle = item->rot.y;
        } else if (
            g_Input.left
            && Lara_FloorFront(item, item->rot.y - DEG_90, STEP_L)
                >= -STEPUP_HEIGHT) {
            item->goal_anim_state = LS_JUMP_LEFT;
            g_Lara.move_angle = item->rot.y - DEG_90;
        } else if (
            g_Input.right
            && Lara_FloorFront(item, item->rot.y + DEG_90, STEP_L)
                >= -STEPUP_HEIGHT) {
            item->goal_anim_state = LS_JUMP_RIGHT;
            g_Lara.move_angle = item->rot.y + DEG_90;
        } else if (
            g_Input.back
            && Lara_FloorFront(item, item->rot.y + DEG_180, STEP_L)
                >= -STEPUP_HEIGHT) {
            item->goal_anim_state = LS_JUMP_BACK;
            g_Lara.move_angle = item->rot.y + DEG_180;
        }
    }

    if (item->fall_speed > LARA_FAST_FALL_SPEED) {
        item->goal_anim_state = LS_FAST_FALL;
    }
}

static void M_Back(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_STOP;
        return;
    }

    if (g_Input.back && (g_Input.slow || g_Lara.water_status == LWS_WADE)) {
        item->goal_anim_state = LS_WALK_BACK;
    } else {
        item->goal_anim_state = LS_STOP;
    }

    if (g_Input.left) {
        g_Lara.turn_rate -= LARA_TURN_RATE;
        CLAMPL(g_Lara.turn_rate, -LARA_SLOW_TURN);
    } else if (g_Input.right) {
        g_Lara.turn_rate += LARA_TURN_RATE;
        CLAMPG(g_Lara.turn_rate, LARA_SLOW_TURN);
    }
}

static void M_Null(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
}

static void M_FastTurn(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_STOP;
        return;
    }

    if (g_Lara.turn_rate >= 0) {
        g_Lara.turn_rate = LARA_FAST_TURN;
        if (!g_Input.right) {
            item->goal_anim_state = LS_STOP;
        }
    } else {
        g_Lara.turn_rate = -LARA_FAST_TURN;
        if (!g_Input.left) {
            item->goal_anim_state = LS_STOP;
        }
    }
}

static void M_StepRight(ITEM *item, COLL_INFO *coll)
{
    g_Lara.enable_look = false;
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_STOP;
        return;
    }

    if (!g_Input.step_right) {
        item->goal_anim_state = LS_STOP;
    }

    if (g_Input.left) {
        g_Lara.turn_rate -= LARA_TURN_RATE;
        CLAMPL(g_Lara.turn_rate, -LARA_SLOW_TURN);
    } else if (g_Input.right) {
        g_Lara.turn_rate += LARA_TURN_RATE;
        CLAMPG(g_Lara.turn_rate, LARA_SLOW_TURN);
    }
}

static void M_StepLeft(ITEM *item, COLL_INFO *coll)
{
    g_Lara.enable_look = false;
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_STOP;
        return;
    }

    if (!g_Input.step_left) {
        item->goal_anim_state = LS_STOP;
    }

    if (g_Input.left) {
        g_Lara.turn_rate -= LARA_TURN_RATE;
        CLAMPL(g_Lara.turn_rate, -LARA_SLOW_TURN);
    } else if (g_Input.right) {
        g_Lara.turn_rate += LARA_TURN_RATE;
        CLAMPG(g_Lara.turn_rate, LARA_SLOW_TURN);
    }
}

static void M_Slide(ITEM *item, COLL_INFO *coll)
{
    g_Camera.flags = CF_NO_CHUNKY;
    g_Camera.target_elevation = CAM_SLIDE_ELEVATION;
    if (g_Input.jump && !g_Input.back) {
        item->goal_anim_state = LS_JUMP_FORWARD;
    }
}

static void M_BackJump(ITEM *item, COLL_INFO *coll)
{
    g_Camera.target_angle = CAM_BACK_JUMP_ANGLE;
    if (item->fall_speed > LARA_FAST_FALL_SPEED) {
        item->goal_anim_state = LS_FAST_FALL;
        return;
    }

    if (item->goal_anim_state == LS_RUN) {
        item->goal_anim_state = LS_STOP;
    } else if (
        (g_Input.forward || g_Input.roll) && item->goal_anim_state != LS_STOP) {
        item->goal_anim_state = LS_TWIST;
    }
}

static void M_RightJump(ITEM *item, COLL_INFO *coll)
{
    g_Lara.enable_look = false;
    if (item->fall_speed > LARA_FAST_FALL_SPEED) {
        item->goal_anim_state = LS_FAST_FALL;
        return;
    }

    if (g_Input.left && item->goal_anim_state != LS_STOP) {
        item->goal_anim_state = LS_TWIST;
    }
}

static void M_LeftJump(ITEM *item, COLL_INFO *coll)
{
    g_Lara.enable_look = false;
    if (item->fall_speed > LARA_FAST_FALL_SPEED) {
        item->goal_anim_state = LS_FAST_FALL;
        return;
    }

    if (g_Input.right && item->goal_anim_state != LS_STOP) {
        item->goal_anim_state = LS_TWIST;
    }
}

static void M_UpJump(ITEM *item, COLL_INFO *coll)
{
    if (item->fall_speed
        > (g_Config.gameplay.enable_swing_cancel ? LARA_SWING_FAST_FALL_SPEED
                                                 : LARA_FAST_FALL_SPEED)) {
        item->goal_anim_state = LS_FAST_FALL;
    }
}

static void M_FallBack(ITEM *item, COLL_INFO *coll)
{
    if (item->fall_speed > LARA_FAST_FALL_SPEED) {
        item->goal_anim_state = LS_FAST_FALL;
        return;
    }

    if (g_Input.action && g_Lara.gun_status == LGS_ARMLESS) {
        item->goal_anim_state = LS_REACH;
    }
}

static void M_HangLeft(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_HANG_ANGLE;
    g_Camera.target_elevation = CAM_HANG_ELEVATION;
    if (!g_Input.left && !g_Input.step_left) {
        item->goal_anim_state = LS_HANG;
    }
}

static void M_HangRight(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_HANG_ANGLE;
    g_Camera.target_elevation = CAM_HANG_ELEVATION;
    if (!g_Input.right && !g_Input.step_right) {
        item->goal_anim_state = LS_HANG;
    }
}

static void M_SlideBack(ITEM *item, COLL_INFO *coll)
{
    if (g_Input.jump && !g_Input.forward) {
        item->goal_anim_state = LS_JUMP_BACK;
    }
}

static void M_PushBlock(ITEM *item, COLL_INFO *coll)
{
    g_Lara.enable_look = false;
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.flags = CF_FOLLOW_CENTRE;
    g_Camera.target_angle = CAM_PUSH_BLOCK_ANGLE;
    g_Camera.target_elevation = CAM_PUSH_BLOCK_ELEVATION;
}

static void M_PPReady(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_PP_READY_ANGLE;
    if (!g_Input.action) {
        item->goal_anim_state = LS_STOP;
    }
}

static void M_Pickup(ITEM *item, COLL_INFO *coll)
{
    g_Lara.enable_look = false;
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_PICKUP_ANGLE;
    g_Camera.target_elevation = CAM_PICKUP_ELEVATION;
    g_Camera.target_distance = CAM_PICKUP_DISTANCE;
}

static void M_PickupFlare(ITEM *item, COLL_INFO *coll)
{
    M_Pickup(item, coll);
    const int16_t frame_num = Item_TestAnimEqual(item, LA_FLARE_PICKUP)
        ? LF_FLARE_PICKUP_END
        : LF_UW_FLARE_PICKUP_END;
    if (Item_TestFrameEqual(item, frame_num)) {
        g_Lara.gun_status = LGS_ARMLESS;
    }
}

static void M_SwitchOn(ITEM *item, COLL_INFO *coll)
{
    g_Lara.enable_look = false;
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_SWITCH_ON_ANGLE;
    g_Camera.target_elevation = CAM_SWITCH_ON_ELEVATION;
    g_Camera.target_distance = CAM_SWITCH_ON_DISTANCE;
    g_Camera.speed = CAM_SWITCH_ON_SPEED;
}

static void M_UseKey(ITEM *item, COLL_INFO *coll)
{
    g_Lara.enable_look = false;
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_USE_KEY_ANGLE;
    g_Camera.target_elevation = CAM_USE_KEY_ELEVATION;
    g_Camera.target_distance = CAM_USE_KEY_DISTANCE;
}

static void M_Special(ITEM *item, COLL_INFO *coll)
{
    g_Camera.flags = CF_FOLLOW_CENTRE;
    g_Camera.target_angle = CAM_SPECIAL_ANGLE;
    g_Camera.target_elevation = CAM_SPECIAL_ELEVATION;
}

static void M_SwanDive(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 1;
    if (item->fall_speed > LARA_FAST_FALL_SPEED
        && item->goal_anim_state != LS_DIVE) {
        item->goal_anim_state = LS_FAST_DIVE;
    }
}

static void M_FastDive(ITEM *item, COLL_INFO *coll)
{
    if (g_Input.roll && item->goal_anim_state == LS_FAST_DIVE) {
        item->goal_anim_state = LS_TWIST;
    }
    coll->enable_hit = 0;
    coll->enable_baddie_push = 1;
    item->speed = item->speed * 95 / 100;
}

static void M_Wade(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_STOP;
        return;
    }

    g_Camera.target_elevation = CAM_WADE_ELEVATION;
    if (g_Input.left) {
        g_Lara.turn_rate -= LARA_TURN_RATE;
        CLAMPL(g_Lara.turn_rate, -LARA_FAST_TURN);
        item->rot.z -= LARA_LEAN_RATE;
        CLAMPL(item->rot.z, -LARA_LEAN_MAX);
    } else if (g_Input.right) {
        g_Lara.turn_rate += LARA_TURN_RATE;
        CLAMPG(g_Lara.turn_rate, LARA_FAST_TURN);
        item->rot.z += LARA_LEAN_RATE;
        CLAMPG(item->rot.z, LARA_LEAN_MAX);
    }

    if (g_Input.forward) {
        if (g_Lara.water_status != LWS_ABOVE_WATER) {
            item->goal_anim_state = LS_WADE;
        } else {
            item->goal_anim_state = LS_RUN;
        }
    } else {
        item->goal_anim_state = LS_STOP;
    }
}

static void M_Zipline(ITEM *item, COLL_INFO *coll)
{
    g_Camera.target_angle = CAM_ZIPLINE_ANGLE;

    if (!g_Input.action) {
        item->goal_anim_state = LS_JUMP_FORWARD;
        Lara_Animate(item);
        g_LaraItem->gravity = 1;
        g_LaraItem->speed = 100;
        g_LaraItem->fall_speed = 40;
        g_Lara.move_angle = item->rot.y;
    }
}

static void M_ClimbLeft(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_CLIMB_LEFT_ANGLE;
    g_Camera.target_elevation = CAM_CLIMB_LEFT_ELEVATION;
    if (!g_Input.left && !g_Input.step_left) {
        item->goal_anim_state = LS_CLIMB_STANCE;
    }
}

static void M_ClimbRight(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_CLIMB_RIGHT_ANGLE;
    g_Camera.target_elevation = CAM_CLIMB_RIGHT_ELEVATION;
    if (!g_Input.right && !g_Input.step_right) {
        item->goal_anim_state = LS_CLIMB_STANCE;
    }
}

static void M_ClimbStance(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_elevation = CAM_CLIMB_STANCE_ELEVATION;

    if (g_Input.look) {
        Lara_LookUpDown();
    }

    if (g_Input.left || g_Input.step_left) {
        item->goal_anim_state = LS_CLIMB_LEFT;
    } else if (g_Input.right || g_Input.step_right) {
        item->goal_anim_state = LS_CLIMB_RIGHT;
    } else if (g_Input.jump) {
        item->goal_anim_state = LS_JUMP_BACK;
        g_Lara.gun_status = LGS_ARMLESS;
        g_Lara.move_angle = item->rot.y + DEG_180;
    }
}

static void M_Climbing(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_elevation = CAM_CLIMBING_ELEVATION;
}

static void M_ClimbEnd(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.flags = CF_FOLLOW_CENTRE;
    g_Camera.target_angle = CAM_CLIMB_END_ELEVATION;
}

static void M_ClimbDown(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_elevation = CAM_CLIMB_DOWN_ELEVATION;
}

static void M_SurfBack(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_UW_DEATH;
        return;
    }

    g_Lara.dive_timer = 0;
    if (!g_Config.input.enable_tr3_sidesteps || !g_Input.slow) {
        if (g_Input.left) {
            item->rot.y -= LARA_SURF_TURN;
        } else if (g_Input.right) {
            item->rot.y += LARA_SURF_TURN;
        }
    }
    if (!g_Input.back) {
        item->goal_anim_state = LS_SURF_TREAD;
    }
    item->fall_speed += 8;
    CLAMPG(item->fall_speed, LARA_MAX_SURF_SPEED);
}

static void M_SurfLeft(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_UW_DEATH;
        return;
    }

    g_Lara.dive_timer = 0;
    if (!g_Config.input.enable_tr3_sidesteps || !g_Input.slow) {
        if (g_Input.left) {
            item->rot.y -= LARA_SURF_TURN;
        } else if (g_Input.right) {
            item->rot.y += LARA_SURF_TURN;
        }
        if (!g_Input.step_left) {
            item->goal_anim_state = LS_SURF_TREAD;
        }
    }
    item->fall_speed += 8;
    CLAMPG(item->fall_speed, LARA_MAX_SURF_SPEED);
}

static void M_SurfRight(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_UW_DEATH;
        return;
    }

    g_Lara.dive_timer = 0;
    if (!g_Config.input.enable_tr3_sidesteps || !g_Input.slow) {
        if (g_Input.left) {
            item->rot.y -= LARA_SURF_TURN;
        } else if (g_Input.right) {
            item->rot.y += LARA_SURF_TURN;
        }
        if (!g_Input.step_right) {
            item->goal_anim_state = LS_SURF_TREAD;
        }
    }
    item->fall_speed += 8;
    CLAMPG(item->fall_speed, LARA_MAX_SURF_SPEED);
}

static void M_SurfTread(ITEM *item, COLL_INFO *coll)
{
    item->fall_speed -= 4;
    CLAMPL(item->fall_speed, 0);

    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_UW_DEATH;
        return;
    }
    if (g_Input.look) {
        Lara_LookUpDown();
        return;
    }

    if (g_Input.left) {
        item->rot.y -= LARA_SLOW_TURN;
    } else if (g_Input.right) {
        item->rot.y += LARA_SLOW_TURN;
    }

    if (g_Input.forward) {
        item->goal_anim_state = LS_SURF_SWIM;
    } else if (g_Input.back) {
        item->goal_anim_state = LS_SURF_BACK;
    }

    if (g_Input.step_left) {
        item->goal_anim_state = LS_SURF_LEFT;
    } else if (g_Input.step_right) {
        item->goal_anim_state = LS_SURF_RIGHT;
    }

    if (g_Input.jump) {
        g_Lara.dive_timer++;
        if (g_Lara.dive_timer == LARA_DIVE_WAIT) {
            Item_SwitchToAnim(item, LA_ONWATER_DIVE, 0);
            item->goal_anim_state = LS_SWIM;
            item->current_anim_state = LS_DIVE;
            item->rot.x = -45 * DEG_1;
            item->fall_speed = 80;
            g_Lara.water_status = LWS_UNDERWATER;
        }
    } else {
        g_Lara.dive_timer = 0;
    }
}

static void M_Swim(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_UW_DEATH;
        return;
    }

    if (g_Input.roll) {
        item->current_anim_state = LS_WATER_ROLL;
        Item_SwitchToAnim(item, LA_UNDERWATER_ROLL_START, 0);
        return;
    }

    M_SwimTurn(item);
    item->fall_speed += 8;
    if (g_Lara.water_status == LWS_CHEAT) {
        CLAMPG(item->fall_speed, LARA_MAX_SWIM_SPEED * 2);
    } else {
        CLAMPG(item->fall_speed, LARA_MAX_SWIM_SPEED);
    }

    if (!g_Input.jump) {
        item->goal_anim_state = LS_GLIDE;
    }
}

static void M_Glide(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_UW_DEATH;
        return;
    }

    if (g_Input.roll) {
        item->current_anim_state = LS_WATER_ROLL;
        Item_SwitchToAnim(item, LA_UNDERWATER_ROLL_START, 0);
        return;
    }

    M_SwimTurn(item);
    if (g_Input.jump) {
        item->goal_anim_state = LS_SWIM;
    }
    item->fall_speed -= LARA_UW_FRICTION;
    CLAMPL(item->fall_speed, 0);
    if (item->fall_speed <= LARA_MAX_SWIM_SPEED * 2 / 3) {
        item->goal_anim_state = LS_TREAD;
    }
}

static void M_Tread(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_UW_DEATH;
        return;
    }

    if (g_Input.roll) {
        item->current_anim_state = LS_WATER_ROLL;
        Item_SwitchToAnim(item, LA_UNDERWATER_ROLL_START, 0);
        return;
    }

    if (g_Input.look) {
        Lara_LookUpDown();
    }
    M_SwimTurn(item);
    if (g_Input.jump) {
        item->goal_anim_state = LS_SWIM;
    }
    item->fall_speed -= LARA_UW_FRICTION;
    CLAMPL(item->fall_speed, 0);
    if (g_Lara.gun_status == LGS_HANDS_BUSY) {
        g_Lara.gun_status = LGS_ARMLESS;
    }
}

static void M_UWDeath(ITEM *item, COLL_INFO *coll)
{
    item->gravity = false;
    item->fall_speed -= 8;
    CLAMPL(item->fall_speed, 0);

    int32_t angle = 2 * DEG_1;
    if (item->rot.x >= -angle && item->rot.x <= angle) {
        item->rot.x = 0;
    } else if (item->rot.x >= 0) {
        item->rot.x -= angle;
    } else {
        item->rot.x += angle;
    }
}

// clang-format off
REGISTER_LARA_STATE(LS_WALK,         M_Walk)
REGISTER_LARA_STATE(LS_RUN,          M_Run)
REGISTER_LARA_STATE(LS_STOP,         M_Stop)
REGISTER_LARA_STATE(LS_JUMP_FORWARD, M_ForwardJump)
REGISTER_LARA_STATE(LS_FAST_BACK,    M_FastBack)
REGISTER_LARA_STATE(LS_TURN_RIGHT,   M_TurnRight)
REGISTER_LARA_STATE(LS_TURN_LEFT,    M_TurnLeft)
REGISTER_LARA_STATE(LS_DEATH,        M_Death)
REGISTER_LARA_STATE(LS_FAST_FALL,    M_FastFall)
REGISTER_LARA_STATE(LS_HANG,         M_Hang)
REGISTER_LARA_STATE(LS_REACH,        M_Reach)
REGISTER_LARA_STATE(LS_SPLAT,        M_Splat)
REGISTER_LARA_STATE(LS_TREAD,        M_Tread)
REGISTER_LARA_STATE(LS_COMPRESS,     M_Compress)
REGISTER_LARA_STATE(LS_WALK_BACK,    M_Back)
REGISTER_LARA_STATE(LS_SWIM,         M_Swim)
REGISTER_LARA_STATE(LS_GLIDE,        M_Glide)
REGISTER_LARA_STATE(LS_PULL_UP,      M_Null)
REGISTER_LARA_STATE(LS_FAST_TURN,    M_FastTurn)
REGISTER_LARA_STATE(LS_STEP_RIGHT,   M_StepRight)
REGISTER_LARA_STATE(LS_STEP_LEFT,    M_StepLeft)
REGISTER_LARA_STATE(LS_SLIDE,        M_Slide)
REGISTER_LARA_STATE(LS_JUMP_BACK,    M_BackJump)
REGISTER_LARA_STATE(LS_JUMP_RIGHT,   M_RightJump)
REGISTER_LARA_STATE(LS_JUMP_LEFT,    M_LeftJump)
REGISTER_LARA_STATE(LS_JUMP_UP,      M_UpJump)
REGISTER_LARA_STATE(LS_FALL_BACK,    M_FallBack)
REGISTER_LARA_STATE(LS_SHIMMY_LEFT,  M_HangLeft)
REGISTER_LARA_STATE(LS_SHIMMY_RIGHT, M_HangRight)
REGISTER_LARA_STATE(LS_SLIDE_BACK,   M_SlideBack)
REGISTER_LARA_STATE(LS_SURF_TREAD,   M_SurfTread)
REGISTER_LARA_STATE(LS_PUSH_BLOCK,   M_PushBlock)
REGISTER_LARA_STATE(LS_PULL_BLOCK,   M_PushBlock)
REGISTER_LARA_STATE(LS_PP_READY,     M_PPReady)
REGISTER_LARA_STATE(LS_PICKUP,       M_Pickup)
REGISTER_LARA_STATE(LS_SWITCH_ON,    M_SwitchOn)
REGISTER_LARA_STATE(LS_SWITCH_OFF,   M_SwitchOn)
REGISTER_LARA_STATE(LS_USE_KEY,      M_UseKey)
REGISTER_LARA_STATE(LS_USE_PUZZLE,   M_UseKey)
REGISTER_LARA_STATE(LS_UW_DEATH,     M_UWDeath)
REGISTER_LARA_STATE(LS_SPECIAL,      M_Special)
REGISTER_LARA_STATE(LS_SURF_BACK,    M_SurfBack)
REGISTER_LARA_STATE(LS_SURF_LEFT,    M_SurfLeft)
REGISTER_LARA_STATE(LS_SURF_RIGHT,   M_SurfRight)
REGISTER_LARA_STATE(LS_SWAN_DIVE,    M_SwanDive)
REGISTER_LARA_STATE(LS_FAST_DIVE,    M_FastDive)
REGISTER_LARA_STATE(LS_GYMNAST,      M_Null)
REGISTER_LARA_STATE(LS_CLIMB_STANCE, M_ClimbStance)
REGISTER_LARA_STATE(LS_CLIMBING,     M_Climbing)
REGISTER_LARA_STATE(LS_CLIMB_LEFT,   M_ClimbLeft)
REGISTER_LARA_STATE(LS_CLIMB_END,    M_ClimbEnd)
REGISTER_LARA_STATE(LS_CLIMB_RIGHT,  M_ClimbRight)
REGISTER_LARA_STATE(LS_CLIMB_DOWN,   M_ClimbDown)
REGISTER_LARA_STATE(LS_WADE,         M_Wade)
REGISTER_LARA_STATE(LS_FLARE_PICKUP, M_PickupFlare)
REGISTER_LARA_STATE(LS_ZIPLINE,      M_Zipline)
// clang-format on
