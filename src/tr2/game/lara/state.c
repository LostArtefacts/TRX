#include "game/lara/state.h"

#include "game/input.h"
#include "game/inventory.h"
#include "game/lara/control.h"
#include "game/lara/look.h"
#include "game/lara/misc.h"
#include "game/sound.h"
#include "game/viewport.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/game.h>
#include <libtrx/game/music.h>
#include <libtrx/utils.h>

#define LF_ROLL 2
#define LF_JUMP_READY 4
#define LF_FLARE_PICKUP_END 89
#define LF_UW_FLARE_PICKUP_END 35
#define LF_SHARK_DEATH_END 56
#define LF_SHARK_DEATH_TIMER_DELAY 25
#define LF_TREX_DEATH_TIMER_DELAY 45
#define LF_YETI_DEATH_TIMER_DELAY 70
#define LF_DRAGON_DAGGER_PULLED 1
#define LF_DRAGON_DAGGER_STORED 180
#define LF_DRAGON_DAGGER_ANIM_END 239
#define LF_START_HOUSE_BEGIN 1
#define LF_START_HOUSE_DAGGER_STORED 401
#define LF_START_HOUSE_END 427
#define LF_SHOWER_START 1
#define LF_SHOWER_SHOTGUN_PICKUP 316
#define LF_SHOWER_END 349

static bool m_JumpPermitted = true;

void Lara_SwimTurn(ITEM *const item)
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

void Lara_State_Empty(ITEM *item, COLL_INFO *coll)
{
}

void Lara_State_Walk(ITEM *item, COLL_INFO *coll)
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

void Lara_State_Run(ITEM *item, COLL_INFO *coll)
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

void Lara_State_Stop(ITEM *item, COLL_INFO *coll)
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
                Lara_State_Wade(item, coll);
            } else {
                Lara_State_Walk(item, coll);
            }
        } else if (g_Input.back) {
            Lara_State_Back(item, coll);
        }
    } else if (g_Input.jump) {
        item->goal_anim_state = LS_COMPRESS;
    } else if (g_Input.forward) {
        if (g_Input.slow) {
            Lara_State_Walk(item, coll);
        } else {
            Lara_State_Run(item, coll);
        }
    } else if (g_Input.back) {
        if (g_Input.slow) {
            Lara_State_Back(item, coll);
        } else {
            item->goal_anim_state = LS_FAST_BACK;
        }
    }
}

void Lara_State_ForwardJump(ITEM *item, COLL_INFO *coll)
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

void Lara_State_FastBack(ITEM *item, COLL_INFO *coll)
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

void Lara_State_TurnRight(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_STOP;
        return;
    }

    g_Lara.turn_rate += LARA_TURN_RATE;

    if (g_Lara.gun_status == LWS_WADE) {
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

void Lara_State_TurnLeft(ITEM *item, COLL_INFO *coll)
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

void Lara_State_Death(ITEM *item, COLL_INFO *coll)
{
    g_Lara.look = 0;
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
}

void Lara_State_FastFall(ITEM *item, COLL_INFO *coll)
{
    item->speed = item->speed * 95 / 100;
    if (item->fall_speed == DAMAGE_START + DAMAGE_LENGTH) {
        Sound_Effect(SFX_LARA_FALL, &item->pos, SPM_NORMAL);
    }
}

void Lara_State_Hang(ITEM *item, COLL_INFO *coll)
{
    if (g_Input.look) {
        Lara_LookUpDown();
    }

    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_HANG_ANGLE;
    g_Camera.target_elevation = CAM_HANG_ELEVATION;
    if (g_Input.left || g_Input.step_left) {
        item->goal_anim_state = LS_HANG_LEFT;
    } else if (g_Input.right || g_Input.step_right) {
        item->goal_anim_state = LS_HANG_RIGHT;
    }
}

void Lara_State_Reach(ITEM *item, COLL_INFO *coll)
{
    g_Camera.target_angle = CAM_REACH_ANGLE;
    if (item->fall_speed > LARA_FAST_FALL_SPEED) {
        item->goal_anim_state = LS_FAST_FALL;
    }
}

void Lara_State_Splat(ITEM *item, COLL_INFO *coll)
{
    g_Lara.look = 0;
}

void Lara_State_Compress(ITEM *item, COLL_INFO *coll)
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

void Lara_State_Back(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_STOP;
        return;
    }

    if (g_Input.back && (g_Input.slow || g_Lara.water_status == LWS_WADE)) {
        item->goal_anim_state = LS_BACK;
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

void Lara_State_Null(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
}

void Lara_State_FastTurn(ITEM *item, COLL_INFO *coll)
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

void Lara_State_StepRight(ITEM *item, COLL_INFO *coll)
{
    g_Lara.look = 0;
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

void Lara_State_StepLeft(ITEM *item, COLL_INFO *coll)
{
    g_Lara.look = 0;
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

void Lara_State_Slide(ITEM *item, COLL_INFO *coll)
{
    g_Camera.flags = CF_NO_CHUNKY;
    g_Camera.target_elevation = CAM_SLIDE_ELEVATION;
    if (g_Input.jump && !g_Input.back) {
        item->goal_anim_state = LS_JUMP_FORWARD;
    }
}

void Lara_State_BackJump(ITEM *item, COLL_INFO *coll)
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

void Lara_State_RightJump(ITEM *item, COLL_INFO *coll)
{
    g_Lara.look = 0;
    if (item->fall_speed > LARA_FAST_FALL_SPEED) {
        item->goal_anim_state = LS_FAST_FALL;
        return;
    }

    if (g_Input.left && item->goal_anim_state != LS_STOP) {
        item->goal_anim_state = LS_TWIST;
    }
}

void Lara_State_LeftJump(ITEM *item, COLL_INFO *coll)
{
    g_Lara.look = 0;
    if (item->fall_speed > LARA_FAST_FALL_SPEED) {
        item->goal_anim_state = LS_FAST_FALL;
        return;
    }

    if (g_Input.right && item->goal_anim_state != LS_STOP) {
        item->goal_anim_state = LS_TWIST;
    }
}

void Lara_State_UpJump(ITEM *item, COLL_INFO *coll)
{
    if (item->fall_speed > LARA_FAST_FALL_SPEED) {
        item->goal_anim_state = LS_FAST_FALL;
    }
}

void Lara_State_Fallback(ITEM *item, COLL_INFO *coll)
{
    if (item->fall_speed > LARA_FAST_FALL_SPEED) {
        item->goal_anim_state = LS_FAST_FALL;
        return;
    }

    if (g_Input.action && g_Lara.gun_status == LGS_ARMLESS) {
        item->goal_anim_state = LS_REACH;
    }
}

void Lara_State_HangLeft(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_HANG_ANGLE;
    g_Camera.target_elevation = CAM_HANG_ELEVATION;
    if (!g_Input.left && !g_Input.step_left) {
        item->goal_anim_state = LS_HANG;
    }
}

void Lara_State_HangRight(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_HANG_ANGLE;
    g_Camera.target_elevation = CAM_HANG_ELEVATION;
    if (!g_Input.right && !g_Input.step_right) {
        item->goal_anim_state = LS_HANG;
    }
}

void Lara_State_SlideBack(ITEM *item, COLL_INFO *coll)
{
    if (g_Input.jump && !g_Input.forward) {
        item->goal_anim_state = LS_JUMP_BACK;
    }
}

void Lara_State_PushBlock(ITEM *item, COLL_INFO *coll)
{
    g_Lara.look = 0;
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.flags = CF_FOLLOW_CENTRE;
    g_Camera.target_angle = CAM_PUSH_BLOCK_ANGLE;
    g_Camera.target_elevation = CAM_PUSH_BLOCK_ELEVATION;
}

void Lara_State_PPReady(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_PP_READY_ANGLE;
    if (!g_Input.action) {
        item->goal_anim_state = LS_STOP;
    }
}

void Lara_State_Pickup(ITEM *item, COLL_INFO *coll)
{
    g_Lara.look = 0;
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_PICKUP_ANGLE;
    g_Camera.target_elevation = CAM_PICKUP_ELEVATION;
    g_Camera.target_distance = CAM_PICKUP_DISTANCE;
}

void Lara_State_PickupFlare(ITEM *item, COLL_INFO *coll)
{
    Lara_State_Pickup(item, coll);
    const int16_t frame_num = Item_TestAnimEqual(item, LA_FLARE_PICKUP)
        ? LF_FLARE_PICKUP_END
        : LF_UW_FLARE_PICKUP_END;
    if (Item_TestFrameEqual(item, frame_num)) {
        g_Lara.gun_status = LGS_ARMLESS;
    }
}

void Lara_State_SwitchOn(ITEM *item, COLL_INFO *coll)
{
    g_Lara.look = 0;
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_SWITCH_ON_ANGLE;
    g_Camera.target_elevation = CAM_SWITCH_ON_ELEVATION;
    g_Camera.target_distance = CAM_SWITCH_ON_DISTANCE;
    g_Camera.speed = CAM_SWITCH_ON_SPEED;
}

void Lara_State_UseKey(ITEM *item, COLL_INFO *coll)
{
    g_Lara.look = 0;
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_USE_KEY_ANGLE;
    g_Camera.target_elevation = CAM_USE_KEY_ELEVATION;
    g_Camera.target_distance = CAM_USE_KEY_DISTANCE;
}

void Lara_State_Special(ITEM *item, COLL_INFO *coll)
{
    g_Camera.flags = CF_FOLLOW_CENTRE;
    g_Camera.target_angle = CAM_SPECIAL_ANGLE;
    g_Camera.target_elevation = CAM_SPECIAL_ELEVATION;
}

void Lara_State_SwanDive(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 1;
    if (item->fall_speed > LARA_FAST_FALL_SPEED
        && item->goal_anim_state != LS_DIVE) {
        item->goal_anim_state = LS_FAST_DIVE;
    }
}

void Lara_State_FastDive(ITEM *item, COLL_INFO *coll)
{
    if (g_Input.roll && item->goal_anim_state == LS_FAST_DIVE) {
        item->goal_anim_state = LS_TWIST;
    }
    coll->enable_hit = 0;
    coll->enable_baddie_push = 1;
    item->speed = item->speed * 95 / 100;
}

void Lara_State_WaterOut(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.flags = CF_FOLLOW_CENTRE;
}

void Lara_State_Wade(ITEM *item, COLL_INFO *coll)
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

void Lara_State_Zipline(ITEM *item, COLL_INFO *coll)
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

void Lara_State_Extra_Breath(ITEM *item, COLL_INFO *coll)
{
    Item_SwitchToAnim(item, LA_STAND_IDLE, 0);
    item->goal_anim_state = LS_STOP;
    item->current_anim_state = LS_STOP;
    g_Lara.extra_anim = 0;
    g_Lara.gun_status = LGS_ARMLESS;
    if (g_Camera.type != CAM_HEAVY) {
        g_Camera.type = CAM_CHASE;
    }
    Viewport_AlterFOV(-1);
}

void Lara_State_Extra_YetiKill(ITEM *item, COLL_INFO *coll)
{
    g_Camera.target_angle = CAM_YETI_KILL_ANGLE;
    g_Camera.target_distance = CAM_YETI_KILL_DISTANCE;
    g_Lara.hit_direction = -1;
    if (Item_TestFrameRange(item, 0, LF_YETI_DEATH_TIMER_DELAY)) {
        g_Lara.death_timer = 1;
    }
}

void Lara_State_Extra_SharkKill(ITEM *item, COLL_INFO *coll)
{
    g_Camera.target_angle = CAM_SHARK_KILL_ANGLE;
    g_Camera.target_distance = CAM_SHARK_KILL_DISTANCE;
    g_Lara.hit_direction = -1;

    if (Item_TestFrameEqual(item, LF_SHARK_DEATH_END)) {
        const int32_t water_height = Room_GetWaterHeight(
            item->pos.x, item->pos.y, item->pos.z, item->room_num);
        if (water_height != NO_HEIGHT && water_height < item->pos.y - 100) {
            item->pos.y -= 5;
        }
    }

    if (Item_TestFrameRange(item, 0, LF_SHARK_DEATH_TIMER_DELAY)) {
        g_Lara.death_timer = 1;
    }
}

void Lara_State_Extra_Airlock(ITEM *item, COLL_INFO *coll)
{
    g_Camera.target_angle = CAM_AIRLOCK_ANGLE;
    g_Camera.target_elevation = CAM_AIRLOCK_ELEVATION;
}

void Lara_State_Extra_GongBong(ITEM *item, COLL_INFO *coll)
{
    g_Camera.target_angle = CAM_GONG_BONG_ANGLE;
    g_Camera.target_elevation = CAM_GONG_BONG_ELEVATION;
    g_Camera.target_distance = CAM_GONG_BONG_DISTANCE;
}

void Lara_State_Extra_DinoKill(ITEM *item, COLL_INFO *coll)
{
    g_Camera.flags = CF_FOLLOW_CENTRE;
    g_Camera.target_angle = CAM_TREX_KILL_ANGLE;
    g_Camera.target_elevation = CAM_TREX_KILL_ELEVATION;
    g_Lara.hit_direction = -1;
    if (Item_TestFrameRange(item, 0, LF_TREX_DEATH_TIMER_DELAY)) {
        g_Lara.death_timer = 1;
    }
}

void Lara_State_Extra_PullDagger(ITEM *item, COLL_INFO *coll)
{
    if (Item_TestFrameEqual(item, LF_DRAGON_DAGGER_PULLED)) {
        Music_Play(MX_DAGGER_PULL, MPM_ALWAYS);
    } else if (Item_TestFrameEqual(item, LF_DRAGON_DAGGER_STORED)) {
        Lara_SwapSingleMesh(LM_HAND_R, O_LARA);
        Inv_AddItem(O_PUZZLE_ITEM_2);
    } else if (Item_TestFrameEqual(item, LF_DRAGON_DAGGER_ANIM_END)) {
        item->rot.y += DEG_90;

        const ITEM *const dragon_bones = Item_Find(O_DRAGON_BONES_2);
        if (dragon_bones != nullptr) {
            Room_TestTriggers(dragon_bones);
        }
    }
}

void Lara_State_Extra_StartAnim(ITEM *item, COLL_INFO *coll)
{
    Room_TestTriggers(item);
}

void Lara_State_Extra_StartHouse(ITEM *item, COLL_INFO *coll)
{
    if (Item_TestFrameEqual(item, LF_START_HOUSE_BEGIN)) {
        Music_Play(MX_REVEAL_2, MPM_ALWAYS);
        Lara_SwapSingleMesh(LM_HAND_R, O_LARA_EXTRA);
        Lara_SwapSingleMesh(LM_HIPS, O_LARA_EXTRA);
    } else if (Item_TestFrameEqual(item, LF_START_HOUSE_DAGGER_STORED)) {
        Lara_SwapSingleMesh(LM_HAND_R, O_LARA);
        Lara_SwapSingleMesh(LM_HIPS, O_LARA);
        Inv_AddItem(O_PUZZLE_ITEM_1);
    } else if (Item_TestFrameEqual(item, LF_START_HOUSE_END)) {
        g_Camera.type = CAM_CHASE;
        Viewport_AlterFOV(-1);
    }
}

void Lara_State_Extra_FinalAnim(ITEM *item, COLL_INFO *coll)
{
    item->hit_points = 1000;

    if (Item_TestFrameEqual(item, LF_SHOWER_START)) {
        g_Lara.back_gun = 0;
        Lara_SwapSingleMesh(LM_HAND_R, O_LARA);
        Lara_SwapSingleMesh(LM_HEAD, O_LARA);
        Lara_SwapSingleMesh(LM_HIPS, O_LARA_EXTRA);
        Music_Play(MX_CUTSCENE_BATH, MPM_ALWAYS);
    } else if (Item_TestFrameEqual(item, LF_SHOWER_SHOTGUN_PICKUP)) {
        Lara_SwapSingleMesh(LM_HAND_R, O_LARA_SHOTGUN);
    } else if (Item_TestFrameEqual(item, LF_SHOWER_END)) {
        Game_SetIsLevelComplete(true);
    }
}

void Lara_State_ClimbLeft(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_CLIMB_LEFT_ANGLE;
    g_Camera.target_elevation = CAM_CLIMB_LEFT_ELEVATION;
    if (!g_Input.left && !g_Input.step_left) {
        item->goal_anim_state = LS_CLIMB_STANCE;
    }
}

void Lara_State_ClimbRight(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_CLIMB_RIGHT_ANGLE;
    g_Camera.target_elevation = CAM_CLIMB_RIGHT_ELEVATION;
    if (!g_Input.right && !g_Input.step_right) {
        item->goal_anim_state = LS_CLIMB_STANCE;
    }
}

void Lara_State_ClimbStance(ITEM *item, COLL_INFO *coll)
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

void Lara_State_Climbing(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_elevation = CAM_CLIMBING_ELEVATION;
}

void Lara_State_ClimbEnd(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.flags = CF_FOLLOW_CENTRE;
    g_Camera.target_angle = CAM_CLIMB_END_ELEVATION;
}

void Lara_State_ClimbDown(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_elevation = CAM_CLIMB_DOWN_ELEVATION;
}

void Lara_State_SurfSwim(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_UW_DEATH;
        return;
    }

    g_Lara.dive_count = 0;
    if (!g_Config.input.enable_tr3_sidesteps || !g_Input.slow) {
        if (g_Input.left) {
            item->rot.y -= LARA_SLOW_TURN;
        } else if (g_Input.right) {
            item->rot.y += LARA_SLOW_TURN;
        }
    }
    if (!g_Input.forward || g_Input.jump) {
        item->goal_anim_state = LS_SURF_TREAD;
    }
    item->fall_speed += 8;
    CLAMPG(item->fall_speed, LARA_MAX_SURF_SPEED);
}

void Lara_State_SurfBack(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_UW_DEATH;
        return;
    }

    g_Lara.dive_count = 0;
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

void Lara_State_SurfLeft(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_UW_DEATH;
        return;
    }

    g_Lara.dive_count = 0;
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

void Lara_State_SurfRight(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_UW_DEATH;
        return;
    }

    g_Lara.dive_count = 0;
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

void Lara_State_SurfTread(ITEM *item, COLL_INFO *coll)
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
        g_Lara.dive_count++;
        if (g_Lara.dive_count == 10) {
            Item_SwitchToAnim(item, LA_ONWATER_DIVE, 0);
            item->goal_anim_state = LS_SWIM;
            item->current_anim_state = LS_DIVE;
            item->rot.x = -45 * DEG_1;
            item->fall_speed = 80;
            g_Lara.water_status = LWS_UNDERWATER;
        }
    } else {
        g_Lara.dive_count = 0;
    }
}

void Lara_State_Swim(ITEM *item, COLL_INFO *coll)
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

    Lara_SwimTurn(item);
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

void Lara_State_Glide(ITEM *item, COLL_INFO *coll)
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

    Lara_SwimTurn(item);
    if (g_Input.jump) {
        item->goal_anim_state = LS_SWIM;
    }
    item->fall_speed -= LARA_UW_FRICTION;
    CLAMPL(item->fall_speed, 0);
    if (item->fall_speed <= LARA_MAX_SWIM_SPEED * 2 / 3) {
        item->goal_anim_state = LS_TREAD;
    }
}

void Lara_State_Tread(ITEM *item, COLL_INFO *coll)
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
    Lara_SwimTurn(item);
    if (g_Input.jump) {
        item->goal_anim_state = LS_SWIM;
    }
    item->fall_speed -= LARA_UW_FRICTION;
    CLAMPL(item->fall_speed, 0);
    if (g_Lara.gun_status == LGS_HANDS_BUSY) {
        g_Lara.gun_status = LGS_ARMLESS;
    }
}

void Lara_State_Dive(ITEM *item, COLL_INFO *coll)
{
    if (g_Input.forward) {
        item->rot.x -= DEG_1;
    }
}

void Lara_State_UWDeath(ITEM *item, COLL_INFO *coll)
{
    item->gravity = 0;
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

void Lara_State_UWTwist(ITEM *item, COLL_INFO *coll)
{
    item->fall_speed = 0;
}
