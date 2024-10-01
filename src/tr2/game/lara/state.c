#include "game/lara/state.h"

#include "game/input.h"
#include "game/inventory/backpack.h"
#include "game/lara/control.h"
#include "game/lara/look.h"
#include "game/lara/misc.h"
#include "game/music.h"
#include "game/output.h"
#include "game/room.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/funcs.h"
#include "global/vars.h"

#include <libtrx/utils.h>

void __cdecl Lara_SwimTurn(ITEM *const item)
{
    if (g_Input & IN_FORWARD) {
        item->rot.x -= LARA_TURN_RATE_UW;
    } else if (g_Input & IN_BACK) {
        item->rot.x += LARA_TURN_RATE_UW;
    }

    if (g_Input & IN_LEFT) {
        g_Lara.turn_rate -= LARA_TURN_RATE;
        CLAMPL(g_Lara.turn_rate, -LARA_MED_TURN);
        item->rot.z -= LARA_LEAN_RATE_SWIM;
    } else if (g_Input & IN_RIGHT) {
        g_Lara.turn_rate += LARA_TURN_RATE;
        CLAMPG(g_Lara.turn_rate, LARA_MED_TURN);
        item->rot.z += LARA_LEAN_RATE_SWIM;
    }
}

void __cdecl Lara_State_Walk(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_STOP;
    }

    if (g_Input & IN_LEFT) {
        g_Lara.turn_rate -= LARA_TURN_RATE;
        CLAMPL(g_Lara.turn_rate, -LARA_SLOW_TURN);
    } else if (g_Input & IN_RIGHT) {
        g_Lara.turn_rate += LARA_TURN_RATE;
        CLAMPG(g_Lara.turn_rate, +LARA_SLOW_TURN);
    }

    if (g_Input & IN_FORWARD) {
        if (g_Lara.water_status == LWS_WADE) {
            item->goal_anim_state = LS_WADE;
        } else if (g_Input & IN_SLOW) {
            item->goal_anim_state = LS_WALK;
        } else {
            item->goal_anim_state = LS_RUN;
        }
    } else {
        item->goal_anim_state = LS_STOP;
    }
}

void __cdecl Lara_State_Run(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_DEATH;
        return;
    }

    if (g_Input & IN_ROLL) {
        item->anim_num = LA_ROLL_START;
        item->current_anim_state = LS_ROLL;
        item->goal_anim_state = LS_STOP;
        item->frame_num = g_Anims[item->anim_num].frame_base + 2;
        return;
    }

    if (g_Input & IN_LEFT) {
        g_Lara.turn_rate -= LARA_TURN_RATE;
        CLAMPL(g_Lara.turn_rate, -LARA_FAST_TURN);
        item->rot.z -= LARA_LEAN_RATE;
        CLAMPL(item->rot.z, -LARA_LEAN_MAX);
    } else if (g_Input & IN_RIGHT) {
        g_Lara.turn_rate += LARA_TURN_RATE;
        CLAMPG(g_Lara.turn_rate, +LARA_FAST_TURN);
        item->rot.z += LARA_LEAN_RATE;
        CLAMPG(item->rot.z, +LARA_LEAN_MAX);
    }

    if (item->anim_num == LA_RUN_START) {
        g_JumpPermitted = 0;
    } else if (item->anim_num != LA_RUN || item->frame_num == 4) {
        g_JumpPermitted = 1;
    }

    if ((g_Input & IN_JUMP) && g_JumpPermitted && !item->gravity) {
        item->goal_anim_state = LS_FORWARD_JUMP;
    } else if (g_Input & IN_FORWARD) {
        if (g_Lara.water_status == LWS_WADE) {
            item->goal_anim_state = LS_WADE;
        } else if (g_Input & IN_SLOW) {
            item->goal_anim_state = LS_WALK;
        } else {
            item->goal_anim_state = LS_RUN;
        }
    } else {
        item->goal_anim_state = LS_STOP;
    }
}

void __cdecl Lara_State_Stop(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_DEATH;
        return;
    }

    if ((g_Input & IN_ROLL) && g_Lara.water_status != LWS_WADE) {
        item->anim_num = LA_ROLL_START;
        item->current_anim_state = LS_ROLL;
        item->goal_anim_state = LS_STOP;
        item->frame_num = g_Anims[item->anim_num].frame_base + 2;
        return;
    }

    item->goal_anim_state = LS_STOP;
    if (g_Input & IN_LOOK) {
        Lara_LookUpDown();
    }

    if (g_Input & IN_STEP_LEFT) {
        item->goal_anim_state = LS_STEP_LEFT;
    } else if (g_Input & IN_STEP_RIGHT) {
        item->goal_anim_state = LS_STEP_RIGHT;
    } else if (g_Input & IN_LEFT) {
        item->goal_anim_state = LS_TURN_LEFT;
    } else if (g_Input & IN_RIGHT) {
        item->goal_anim_state = LS_TURN_RIGHT;
    }

    if (g_Lara.water_status == LWS_WADE) {
        if (g_Input & IN_JUMP) {
            item->goal_anim_state = LS_COMPRESS;
        }

        if (g_Input & IN_FORWARD) {
            if (g_Input & IN_SLOW) {
                Lara_State_Wade(item, coll);
            } else {
                Lara_State_Walk(item, coll);
            }
        } else if (g_Input & IN_BACK) {
            Lara_State_Back(item, coll);
        }
    } else if (g_Input & IN_JUMP) {
        item->goal_anim_state = LS_COMPRESS;
    } else if (g_Input & IN_FORWARD) {
        if (g_Input & IN_SLOW) {
            Lara_State_Walk(item, coll);
        } else {
            Lara_State_Run(item, coll);
        }
    } else if (g_Input & IN_BACK) {
        if (g_Input & IN_SLOW) {
            Lara_State_Back(item, coll);
        } else {
            item->goal_anim_state = LS_FAST_BACK;
        }
    }
}

void __cdecl Lara_State_ForwardJump(ITEM *item, COLL_INFO *coll)
{
    if (item->goal_anim_state == LS_SWAN_DIVE
        || item->goal_anim_state == LS_REACH) {
        item->goal_anim_state = LS_FORWARD_JUMP;
    }

    if (item->goal_anim_state != LS_DEATH && item->goal_anim_state != LS_STOP
        && item->goal_anim_state != LS_RUN) {
        if ((g_Input & IN_ACTION) && g_Lara.gun_status == LGS_ARMLESS) {
            item->goal_anim_state = LS_REACH;
        }
        if ((g_Input & IN_ROLL) || (g_Input & IN_BACK)) {
            item->goal_anim_state = LS_TWIST;
        }
        if ((g_Input & IN_SLOW) && g_Lara.gun_status == LGS_ARMLESS) {
            item->goal_anim_state = LS_SWAN_DIVE;
        }
        if (item->fall_speed > LARA_FAST_FALL_SPEED) {
            item->goal_anim_state = LS_FAST_FALL;
        }
    }

    if (g_Input & IN_LEFT) {
        g_Lara.turn_rate -= LARA_TURN_RATE;
        CLAMPL(g_Lara.turn_rate, -LARA_JUMP_TURN);
    } else if (g_Input & IN_RIGHT) {
        g_Lara.turn_rate += LARA_TURN_RATE;
        CLAMPG(g_Lara.turn_rate, +LARA_JUMP_TURN);
    }
}

void __cdecl Lara_State_FastBack(ITEM *item, COLL_INFO *coll)
{
    item->goal_anim_state = LS_STOP;
    if (g_Input & IN_LEFT) {
        g_Lara.turn_rate -= LARA_TURN_RATE;
        CLAMPL(g_Lara.turn_rate, -LARA_MED_TURN);
    } else if (g_Input & IN_RIGHT) {
        g_Lara.turn_rate += LARA_TURN_RATE;
        CLAMPG(g_Lara.turn_rate, LARA_MED_TURN);
    }
}

void __cdecl Lara_State_TurnRight(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_STOP;
        return;
    }

    g_Lara.turn_rate += LARA_TURN_RATE;

    if (g_Lara.gun_status == LWS_WADE) {
        item->goal_anim_state = LS_FAST_TURN;
    } else if (g_Lara.turn_rate > LARA_SLOW_TURN) {
        if (g_Input & IN_SLOW) {
            g_Lara.turn_rate = LARA_SLOW_TURN;
        } else {
            item->goal_anim_state = LS_FAST_TURN;
        }
    }

    if (g_Input & IN_FORWARD) {
        if (g_Lara.water_status == LWS_WADE) {
            item->goal_anim_state = LS_WADE;
        } else if (g_Input & IN_SLOW) {
            item->goal_anim_state = LS_WALK;
        } else {
            item->goal_anim_state = LS_RUN;
        }
    } else if (!(g_Input & IN_RIGHT)) {
        item->goal_anim_state = LS_STOP;
    }
}

void __cdecl Lara_State_TurnLeft(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_STOP;
        return;
    }

    g_Lara.turn_rate -= LARA_TURN_RATE;

    if (g_Lara.gun_status == IN_LEFT) {
        item->goal_anim_state = LS_FAST_TURN;
    } else if (g_Lara.turn_rate < -LARA_SLOW_TURN) {
        if (g_Input & IN_SLOW) {
            g_Lara.turn_rate = -LARA_SLOW_TURN;
        } else {
            item->goal_anim_state = LS_FAST_TURN;
        }
    }

    if (g_Input & IN_FORWARD) {
        if (g_Lara.water_status == IN_LEFT) {
            item->goal_anim_state = LS_WADE;
        } else if (g_Input & IN_SLOW) {
            item->goal_anim_state = LS_WALK;
        } else {
            item->goal_anim_state = LS_RUN;
        }
    } else if (!(g_Input & IN_LEFT)) {
        item->goal_anim_state = LS_STOP;
    }
}

void __cdecl Lara_State_Death(ITEM *item, COLL_INFO *coll)
{
    g_Lara.look = 0;
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
}

void __cdecl Lara_State_FastFall(ITEM *item, COLL_INFO *coll)
{
    item->speed = item->speed * 95 / 100;
    if (item->fall_speed == DAMAGE_START + DAMAGE_LENGTH) {
        Sound_Effect(SFX_LARA_FALL, &item->pos, SPM_NORMAL);
    }
}

void __cdecl Lara_State_Hang(ITEM *item, COLL_INFO *coll)
{
    if (g_Input & IN_LOOK) {
        Lara_LookUpDown();
    }

    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_HANG_ANGLE;
    g_Camera.target_elevation = CAM_HANG_ELEVATION;
    if ((g_Input & IN_LEFT) || (g_Input & IN_STEP_LEFT)) {
        item->goal_anim_state = LS_HANG_LEFT;
    } else if ((g_Input & IN_RIGHT) || (g_Input & IN_STEP_RIGHT)) {
        item->goal_anim_state = LS_HANG_RIGHT;
    }
}

void __cdecl Lara_State_Reach(ITEM *item, COLL_INFO *coll)
{
    g_Camera.target_angle = CAM_REACH_ANGLE;
    if (item->fall_speed > LARA_FAST_FALL_SPEED) {
        item->goal_anim_state = LS_FAST_FALL;
    }
}

void __cdecl Lara_State_Splat(ITEM *item, COLL_INFO *coll)
{
    g_Lara.look = 0;
}

void __cdecl Lara_State_Compress(ITEM *item, COLL_INFO *coll)
{
    if (g_Lara.water_status != LWS_WADE) {
        if ((g_Input & IN_FORWARD)
            && Lara_FloorFront(item, item->rot.y, STEP_L) >= -STEPUP_HEIGHT) {
            item->goal_anim_state = LS_FORWARD_JUMP;
            g_Lara.move_angle = item->rot.y;
        } else if (
            (g_Input & IN_LEFT)
            && Lara_FloorFront(item, item->rot.y - PHD_90, STEP_L)
                >= -STEPUP_HEIGHT) {
            item->goal_anim_state = LS_LEFT_JUMP;
            g_Lara.move_angle = item->rot.y - PHD_90;
        } else if (
            (g_Input & IN_RIGHT)
            && Lara_FloorFront(item, item->rot.y + PHD_90, STEP_L)
                >= -STEPUP_HEIGHT) {
            item->goal_anim_state = LS_RIGHT_JUMP;
            g_Lara.move_angle = item->rot.y + PHD_90;
        } else if (
            (g_Input & IN_BACK)
            && Lara_FloorFront(item, item->rot.y + PHD_180, STEP_L)
                >= -STEPUP_HEIGHT) {
            item->goal_anim_state = LS_BACK_JUMP;
            g_Lara.move_angle = item->rot.y + PHD_180;
        }
    }

    if (item->fall_speed > LARA_FAST_FALL_SPEED) {
        item->goal_anim_state = LS_FAST_FALL;
    }
}

void __cdecl Lara_State_Back(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_STOP;
        return;
    }

    if ((g_Input & IN_BACK)
        && ((g_Input & IN_SLOW) || g_Lara.water_status == LWS_WADE)) {
        item->goal_anim_state = LS_BACK;
    } else {
        item->goal_anim_state = LS_STOP;
    }

    if (g_Input & IN_LEFT) {
        g_Lara.turn_rate -= LARA_TURN_RATE;
        CLAMPL(g_Lara.turn_rate, -LARA_SLOW_TURN);
    } else if (g_Input & IN_RIGHT) {
        g_Lara.turn_rate += LARA_TURN_RATE;
        CLAMPG(g_Lara.turn_rate, LARA_SLOW_TURN);
    }
}

void __cdecl Lara_State_Null(ITEM *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
}

void __cdecl Lara_State_FastTurn(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_STOP;
        return;
    }

    if (g_Lara.turn_rate >= 0) {
        g_Lara.turn_rate = LARA_FAST_TURN;
        if (!(g_Input & IN_RIGHT)) {
            item->goal_anim_state = LS_STOP;
        }
    } else {
        g_Lara.turn_rate = -LARA_FAST_TURN;
        if (!(g_Input & IN_LEFT)) {
            item->goal_anim_state = LS_STOP;
        }
    }
}

void __cdecl Lara_State_StepRight(ITEM *item, COLL_INFO *coll)
{
    g_Lara.look = 0;
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_STOP;
        return;
    }

    if (!(g_Input & IN_STEP_RIGHT)) {
        item->goal_anim_state = LS_STOP;
    }

    if (g_Input & IN_LEFT) {
        g_Lara.turn_rate -= LARA_TURN_RATE;
        CLAMPL(g_Lara.turn_rate, -LARA_SLOW_TURN);
    } else if (g_Input & IN_RIGHT) {
        g_Lara.turn_rate += LARA_TURN_RATE;
        CLAMPG(g_Lara.turn_rate, LARA_SLOW_TURN);
    }
}

void __cdecl Lara_State_StepLeft(ITEM *item, COLL_INFO *coll)
{
    g_Lara.look = 0;
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_STOP;
        return;
    }

    if (!(g_Input & IN_STEP_LEFT)) {
        item->goal_anim_state = LS_STOP;
    }

    if (g_Input & IN_LEFT) {
        g_Lara.turn_rate -= LARA_TURN_RATE;
        CLAMPL(g_Lara.turn_rate, -LARA_SLOW_TURN);
    } else if (g_Input & IN_RIGHT) {
        g_Lara.turn_rate += LARA_TURN_RATE;
        CLAMPG(g_Lara.turn_rate, LARA_SLOW_TURN);
    }
}

void __cdecl Lara_State_Slide(ITEM *item, COLL_INFO *coll)
{
    g_Camera.flags = CF_NO_CHUNKY;
    g_Camera.target_elevation = CAM_SLIDE_ELEVATION;
    if ((g_Input & IN_JUMP) && !(g_Input & IN_BACK)) {
        item->goal_anim_state = LS_FORWARD_JUMP;
    }
}

void __cdecl Lara_State_BackJump(ITEM *item, COLL_INFO *coll)
{
    g_Camera.target_angle = CAM_BACK_JUMP_ANGLE;
    if (item->fall_speed > LARA_FAST_FALL_SPEED) {
        item->goal_anim_state = LS_FAST_FALL;
        return;
    }

    if (item->goal_anim_state == LS_RUN) {
        item->goal_anim_state = LS_STOP;
    } else if (
        ((g_Input & IN_FORWARD) || (g_Input & IN_ROLL))
        && item->goal_anim_state != LS_STOP) {
        item->goal_anim_state = LS_TWIST;
    }
}

void __cdecl Lara_State_RightJump(ITEM *item, COLL_INFO *coll)
{
    g_Lara.look = 0;
    if (item->fall_speed > LARA_FAST_FALL_SPEED) {
        item->goal_anim_state = LS_FAST_FALL;
        return;
    }

    if ((g_Input & IN_LEFT) && item->goal_anim_state != LS_STOP) {
        item->goal_anim_state = LS_TWIST;
    }
}

void __cdecl Lara_State_LeftJump(ITEM *item, COLL_INFO *coll)
{
    g_Lara.look = 0;
    if (item->fall_speed > LARA_FAST_FALL_SPEED) {
        item->goal_anim_state = LS_FAST_FALL;
        return;
    }

    if ((g_Input & IN_RIGHT) && item->goal_anim_state != LS_STOP) {
        item->goal_anim_state = LS_TWIST;
    }
}

void __cdecl Lara_State_UpJump(ITEM *item, COLL_INFO *coll)
{
    if (item->fall_speed > LARA_FAST_FALL_SPEED) {
        item->goal_anim_state = LS_FAST_FALL;
    }
}

void __cdecl Lara_State_Fallback(ITEM *item, COLL_INFO *coll)
{
    if (item->fall_speed > LARA_FAST_FALL_SPEED) {
        item->goal_anim_state = LS_FAST_FALL;
        return;
    }

    if ((g_Input & IN_ACTION) && g_Lara.gun_status == LGS_ARMLESS) {
        item->goal_anim_state = LS_REACH;
    }
}

void __cdecl Lara_State_HangLeft(ITEM *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_HANG_ANGLE;
    g_Camera.target_elevation = CAM_HANG_ELEVATION;
    if (!(g_Input & IN_LEFT) && !(g_Input & IN_STEP_LEFT)) {
        item->goal_anim_state = LS_HANG;
    }
}

void __cdecl Lara_State_HangRight(ITEM *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_HANG_ANGLE;
    g_Camera.target_elevation = CAM_HANG_ELEVATION;
    if (!(g_Input & IN_RIGHT) && !(g_Input & IN_STEP_RIGHT)) {
        item->goal_anim_state = LS_HANG;
    }
}

void __cdecl Lara_State_SlideBack(ITEM *item, COLL_INFO *coll)
{
    if ((g_Input & IN_JUMP) && !(g_Input & IN_FORWARD)) {
        item->goal_anim_state = LS_BACK_JUMP;
    }
}

void __cdecl Lara_State_PushBlock(ITEM *item, COLL_INFO *coll)
{
    g_Lara.look = 0;
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    g_Camera.flags = CF_FOLLOW_CENTRE;
    g_Camera.target_angle = CAM_PUSH_BLOCK_ANGLE;
    g_Camera.target_elevation = CAM_PUSH_BLOCK_ELEVATION;
}

void __cdecl Lara_State_PPReady(ITEM *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_PP_READY_ANGLE;
    if (!(g_Input & IN_ACTION)) {
        item->goal_anim_state = LS_STOP;
    }
}

void __cdecl Lara_State_Pickup(ITEM *item, COLL_INFO *coll)
{
    g_Lara.look = 0;
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_PICKUP_ANGLE;
    g_Camera.target_elevation = CAM_PICKUP_ELEVATION;
    g_Camera.target_distance = CAM_PICKUP_DISTANCE;
}

void __cdecl Lara_State_PickupFlare(ITEM *item, COLL_INFO *coll)
{
    Lara_State_Pickup(item, coll);
    if (item->frame_num == g_Anims[item->anim_num].frame_end - 1) {
        g_Lara.gun_status = LGS_ARMLESS;
    }
}

void __cdecl Lara_State_SwitchOn(ITEM *item, COLL_INFO *coll)
{
    g_Lara.look = 0;
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_SWITCH_ON_ANGLE;
    g_Camera.target_elevation = CAM_SWITCH_ON_ELEVATION;
    g_Camera.target_distance = CAM_SWITCH_ON_DISTANCE;
    g_Camera.speed = CAM_SWITCH_ON_SPEED;
}

void __cdecl Lara_State_UseKey(ITEM *item, COLL_INFO *coll)
{
    g_Lara.look = 0;
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_USE_KEY_ANGLE;
    g_Camera.target_elevation = CAM_USE_KEY_ELEVATION;
    g_Camera.target_distance = CAM_USE_KEY_DISTANCE;
}

void __cdecl Lara_State_Special(ITEM *item, COLL_INFO *coll)
{
    g_Camera.flags = CF_FOLLOW_CENTRE;
    g_Camera.target_angle = CAM_SPECIAL_ANGLE;
    g_Camera.target_elevation = CAM_SPECIAL_ELEVATION;
}

void __cdecl Lara_State_SwanDive(ITEM *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 1;
    if (item->fall_speed > LARA_FAST_FALL_SPEED
        && item->goal_anim_state != LS_DIVE) {
        item->goal_anim_state = LS_FAST_DIVE;
    }
}

void __cdecl Lara_State_FastDive(ITEM *item, COLL_INFO *coll)
{
    if ((g_Input & IN_ROLL) && item->goal_anim_state == LS_FAST_DIVE) {
        item->goal_anim_state = LS_TWIST;
    }
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 1;
    item->speed = item->speed * 95 / 100;
}

void __cdecl Lara_State_WaterOut(ITEM *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    g_Camera.flags = CF_FOLLOW_CENTRE;
}

void __cdecl Lara_State_Wade(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_STOP;
        return;
    }

    g_Camera.target_elevation = CAM_WADE_ELEVATION;
    if (g_Input & IN_LEFT) {
        g_Lara.turn_rate -= LARA_TURN_RATE;
        CLAMPL(g_Lara.turn_rate, -LARA_FAST_TURN);
        item->rot.z -= LARA_LEAN_RATE;
        CLAMPL(item->rot.z, -LARA_LEAN_MAX);
    } else if (g_Input & IN_RIGHT) {
        g_Lara.turn_rate += LARA_TURN_RATE;
        CLAMPG(g_Lara.turn_rate, LARA_FAST_TURN);
        item->rot.z += LARA_LEAN_RATE;
        CLAMPG(item->rot.z, LARA_LEAN_MAX);
    }

    if (g_Input & IN_FORWARD) {
        if (g_Lara.water_status != LWS_ABOVE_WATER) {
            item->goal_anim_state = LS_WADE;
        } else {
            item->goal_anim_state = IN_FORWARD;
        }
    } else {
        item->goal_anim_state = LS_STOP;
    }
}

void __cdecl Lara_State_DeathSlide(ITEM *item, COLL_INFO *coll)
{
    g_Camera.target_angle = CAM_DEATH_SLIDE_ANGLE;

    int16_t room_num = item->room_num;
    const SECTOR *sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
    Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);

    coll->trigger = g_TriggerIndex;

    if (!(g_Input & IN_ACTION)) {
        item->goal_anim_state = LS_FORWARD_JUMP;
        Lara_Animate(item);
        g_LaraItem->gravity = 1;
        g_LaraItem->speed = 100;
        g_LaraItem->fall_speed = 40;
        g_Lara.move_angle = item->rot.y;
    }
}

void __cdecl Lara_State_Extra_Breath(ITEM *item, COLL_INFO *coll)
{
    item->anim_num = LA_STAND_IDLE;
    item->frame_num = g_Anims[item->anim_num].frame_base;
    item->goal_anim_state = LS_STOP;
    item->current_anim_state = LS_STOP;
    g_Lara.extra_anim = 0;
    g_Lara.gun_status = LGS_ARMLESS;
    g_Camera.type = CAM_CHASE;
    Output_AlterFOV(GAME_FOV * PHD_DEGREE);
}

void __cdecl Lara_State_Extra_YetiKill(ITEM *item, COLL_INFO *coll)
{
    g_Camera.target_angle = CAM_YETI_KILL_ANGLE;
    g_Camera.target_distance = CAM_YETI_KILL_DISTANCE;
    g_Lara.hit_direction = -1;
    if (item->frame_num < g_Anims[item->anim_num].frame_end - 30) {
        g_Lara.death_timer = 1;
    }
}

void __cdecl Lara_State_Extra_SharkKill(ITEM *item, COLL_INFO *coll)
{
    g_Camera.target_angle = CAM_SHARK_KILL_ANGLE;
    g_Camera.target_distance = CAM_SHARK_KILL_DISTANCE;
    g_Lara.hit_direction = -1;

    if (item->frame_num == g_Anims[item->anim_num].frame_end) {
        int32_t water_height = Room_GetWaterHeight(
            item->pos.x, item->pos.y, item->pos.z, item->room_num);
        if (water_height != NO_HEIGHT && water_height < item->pos.y - 100) {
            item->pos.y -= 5;
        }
    }

    if (item->frame_num < g_Anims[item->anim_num].frame_end - 30) {
        g_Lara.death_timer = 1;
    }
}

void __cdecl Lara_State_Extra_Airlock(ITEM *item, COLL_INFO *coll)
{
    g_Camera.target_angle = CAM_AIRLOCK_ANGLE;
    g_Camera.target_elevation = CAM_AIRLOCK_ELEVATION;
}

void __cdecl Lara_State_Extra_GongBong(ITEM *item, COLL_INFO *coll)
{
    g_Camera.target_angle = CAM_GONG_BONG_ANGLE;
    g_Camera.target_elevation = CAM_GONG_BONG_ELEVATION;
    g_Camera.target_distance = CAM_GONG_BONG_DISTANCE;
}

void __cdecl Lara_State_Extra_DinoKill(ITEM *item, COLL_INFO *coll)
{
    g_Camera.flags = CF_FOLLOW_CENTRE;
    g_Camera.target_angle = CAM_DINO_KILL_ANGLE;
    g_Camera.target_elevation = CAM_DINO_KILL_ELEVATION;
    g_Lara.hit_direction = -1;
    if (item->frame_num < g_Anims[item->anim_num].frame_end - 30) {
        g_Lara.death_timer = 1;
    }
}

void __cdecl Lara_State_Extra_PullDagger(ITEM *item, COLL_INFO *coll)
{
    int32_t frame_num_rel =
        item->frame_num - g_Anims[item->anim_num].frame_base;
    if (frame_num_rel == 1) {
        Music_PlaySynced(MX_DAGGER_PULL);
    } else if (frame_num_rel == 180) {
        g_Lara.mesh_ptrs[LM_HAND_R] =
            g_Meshes[g_Objects[O_LARA].mesh_idx + LM_HAND_R];
        Inv_AddItem(O_PUZZLE_ITEM_2);
    }

    if (item->frame_num == g_Anims[item->anim_num].frame_end) {
        item->rot.y += PHD_90;
        int16_t room_num = item->room_num;
        const SECTOR *sector =
            Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
        Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);
        Room_TestTriggers(g_TriggerIndex, 1);
    }
}

void __cdecl Lara_State_Extra_StartAnim(ITEM *item, COLL_INFO *coll)
{
    int16_t room_num = item->room_num;
    const SECTOR *sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
    Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);
    Room_TestTriggers(g_TriggerIndex, 0);
}

void __cdecl Lara_State_Extra_StartHouse(ITEM *item, COLL_INFO *coll)
{
    int32_t frame_num_rel =
        item->frame_num - g_Anims[item->anim_num].frame_base;
    if (frame_num_rel == 1) {
        Music_PlaySynced(MX_REVEAL_2);
        g_Lara.mesh_ptrs[LM_HAND_R] =
            g_Meshes[g_Objects[O_LARA_EXTRA].mesh_idx + LM_HAND_R];
        g_Lara.mesh_ptrs[LM_HIPS] = g_Meshes[g_Objects[O_LARA_EXTRA].mesh_idx];
    } else if (frame_num_rel == 401) {
        g_Lara.mesh_ptrs[LM_HAND_R] =
            g_Meshes[g_Objects[O_LARA].mesh_idx + LM_HAND_R];
        g_Lara.mesh_ptrs[LM_HIPS] = g_Meshes[g_Objects[O_LARA].mesh_idx];
        Inv_AddItem(O_PUZZLE_ITEM_1);
    } else if (item->frame_num == g_Anims[item->anim_num].frame_end) {
        g_Camera.type = CAM_CHASE;
        Output_AlterFOV(GAME_FOV * PHD_DEGREE);
    }
}

void __cdecl Lara_State_Extra_FinalAnim(ITEM *item, COLL_INFO *coll)
{
    item->hit_points = 1000;

    int32_t frame_num_rel =
        item->frame_num - g_Anims[item->anim_num].frame_base;
    if (frame_num_rel == 1) {
        g_Lara.back_gun = 0;
        g_Lara.mesh_ptrs[LM_HAND_R] =
            g_Meshes[g_Objects[O_LARA].mesh_idx + LM_HAND_R];
        g_Lara.mesh_ptrs[LM_HEAD] =
            g_Meshes[g_Objects[O_LARA].mesh_idx + LM_HEAD];
        g_Lara.mesh_ptrs[LM_HIPS] =
            g_Meshes[g_Objects[O_LARA_EXTRA].mesh_idx + LM_HIPS];
        Music_PlaySynced(MX_CUTSCENE_BATH);
    } else if (frame_num_rel == 316) {
        g_Lara.mesh_ptrs[LM_HAND_R] =
            g_Meshes[g_Objects[O_LARA_SHOTGUN].mesh_idx + LM_HAND_R];
    } else if (item->frame_num == g_Anims[item->anim_num].frame_end - 1) {
        g_LevelComplete = true;
    }
}

void __cdecl Lara_State_ClimbLeft(ITEM *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_CLIMB_LEFT_ANGLE;
    g_Camera.target_elevation = CAM_CLIMB_LEFT_ELEVATION;
    if (!(g_Input & IN_LEFT) && !(g_Input & IN_STEP_LEFT)) {
        item->goal_anim_state = LS_CLIMB_STANCE;
    }
}

void __cdecl Lara_State_ClimbRight(ITEM *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_CLIMB_RIGHT_ANGLE;
    g_Camera.target_elevation = CAM_CLIMB_RIGHT_ELEVATION;
    if (!(g_Input & IN_RIGHT) && !(g_Input & IN_STEP_RIGHT)) {
        item->goal_anim_state = LS_CLIMB_STANCE;
    }
}

void __cdecl Lara_State_ClimbStance(ITEM *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_elevation = CAM_CLIMB_STANCE_ELEVATION;

    if (g_Input & IN_LOOK) {
        Lara_LookUpDown();
    }

    if ((g_Input & IN_LEFT) || (g_Input & IN_STEP_LEFT)) {
        item->goal_anim_state = LS_CLIMB_LEFT;
    } else if ((g_Input & IN_RIGHT) || (g_Input & IN_STEP_RIGHT)) {
        item->goal_anim_state = LS_CLIMB_RIGHT;
    } else if ((g_Input & IN_JUMP)) {
        item->goal_anim_state = LS_BACK_JUMP;
        g_Lara.gun_status = LGS_ARMLESS;
        g_Lara.move_angle = item->rot.y + PHD_180;
    }
}

void __cdecl Lara_State_Climbing(ITEM *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_elevation = CAM_CLIMBING_ELEVATION;
}

void __cdecl Lara_State_ClimbEnd(ITEM *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    g_Camera.flags = CF_FOLLOW_CENTRE;
    g_Camera.target_angle = CAM_CLIMB_END_ELEVATION;
}

void __cdecl Lara_State_ClimbDown(ITEM *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_elevation = CAM_CLIMB_DOWN_ELEVATION;
}

void __cdecl Lara_State_SurfSwim(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_UW_DEATH;
        return;
    }

    g_Lara.dive_count = 0;
    if (g_Input & IN_LEFT) {
        item->rot.y -= LARA_SLOW_TURN;
    } else if (g_Input & IN_RIGHT) {
        item->rot.y += LARA_SLOW_TURN;
    }
    if (!(g_Input & LS_RUN) || (g_Input & LS_BACK)) {
        item->goal_anim_state = LS_SURF_TREAD;
    }
    item->fall_speed += 8;
    CLAMPG(item->fall_speed, LARA_MAX_SURF_SPEED);
}

void __cdecl Lara_State_SurfBack(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_UW_DEATH;
        return;
    }

    g_Lara.dive_count = 0;
    if (g_Input & IN_LEFT) {
        item->rot.y -= LARA_SURF_TURN;
    } else if (g_Input & IN_RIGHT) {
        item->rot.y += LARA_SURF_TURN;
    }
    if (!(g_Input & IN_BACK)) {
        item->goal_anim_state = LS_SURF_TREAD;
    }
    item->fall_speed += 8;
    CLAMPG(item->fall_speed, LARA_MAX_SURF_SPEED);
}

void __cdecl Lara_State_SurfLeft(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_UW_DEATH;
        return;
    }

    g_Lara.dive_count = 0;
    if (g_Input & IN_LEFT) {
        item->rot.y -= LARA_SURF_TURN;
    } else if (g_Input & IN_RIGHT) {
        item->rot.y += LARA_SURF_TURN;
    }
    if (!(g_Input & IN_STEP_LEFT)) {
        item->goal_anim_state = LS_SURF_TREAD;
    }
    item->fall_speed += 8;
    CLAMPG(item->fall_speed, LARA_MAX_SURF_SPEED);
}

void __cdecl Lara_State_SurfRight(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_UW_DEATH;
        return;
    }

    g_Lara.dive_count = 0;
    if (g_Input & IN_LEFT) {
        item->rot.y -= LARA_SURF_TURN;
    } else if (g_Input & IN_RIGHT) {
        item->rot.y += LARA_SURF_TURN;
    }
    if (!(g_Input & IN_STEP_RIGHT)) {
        item->goal_anim_state = LS_SURF_TREAD;
    }
    item->fall_speed += 8;
    CLAMPG(item->fall_speed, LARA_MAX_SURF_SPEED);
}

void __cdecl Lara_State_SurfTread(ITEM *item, COLL_INFO *coll)
{
    item->fall_speed -= 4;
    CLAMPL(item->fall_speed, 0);

    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_UW_DEATH;
        return;
    }
    if (g_Input & IN_LOOK) {
        Lara_LookUpDown();
        return;
    }

    if (g_Input & IN_LEFT) {
        item->rot.y -= LARA_SLOW_TURN;
    } else if (g_Input & IN_RIGHT) {
        item->rot.y += LARA_SLOW_TURN;
    }

    if (g_Input & IN_FORWARD) {
        item->goal_anim_state = LS_SURF_SWIM;
    } else if (g_Input & IN_BACK) {
        item->goal_anim_state = LS_SURF_BACK;
    }

    if (g_Input & IN_STEP_LEFT) {
        item->goal_anim_state = LS_SURF_LEFT;
    } else if (g_Input & IN_STEP_RIGHT) {
        item->goal_anim_state = LS_SURF_RIGHT;
    }

    if (g_Input & IN_JUMP) {
        g_Lara.dive_count++;
        if (g_Lara.dive_count == 10) {
            item->anim_num = LA_ONWATER_DIVE;
            item->frame_num = g_Anims[item->anim_num].frame_base;
            item->goal_anim_state = LS_SWIM;
            item->current_anim_state = LS_DIVE;
            item->rot.x = -45 * PHD_DEGREE;
            item->fall_speed = 80;
            g_Lara.water_status = LWS_UNDERWATER;
        }
    } else {
        g_Lara.dive_count = 0;
    }
}

void __cdecl Lara_State_Swim(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_UW_DEATH;
        return;
    }

    if ((g_Input & IN_ROLL)) {
        item->current_anim_state = LS_WATER_ROLL;
        item->anim_num = LA_UNDERWATER_ROLL_START;
        item->frame_num = g_Anims[item->anim_num].frame_base;
        return;
    }

    Lara_SwimTurn(item);
    item->fall_speed += 8;
    if (g_Lara.water_status == LWS_CHEAT) {
        CLAMPG(item->fall_speed, LARA_MAX_SWIM_SPEED * 2);
    } else {
        CLAMPG(item->fall_speed, LARA_MAX_SWIM_SPEED);
    }

    if (!(g_Input & IN_JUMP)) {
        item->goal_anim_state = LS_GLIDE;
    }
}

void __cdecl Lara_State_Glide(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_UW_DEATH;
        return;
    }

    if ((g_Input & IN_ROLL) != 0) {
        item->current_anim_state = LS_WATER_ROLL;
        item->anim_num = LA_UNDERWATER_ROLL_START;
        item->frame_num = g_Anims[item->anim_num].frame_base;
        return;
    }

    Lara_SwimTurn(item);
    if (g_Input & IN_JUMP) {
        item->goal_anim_state = LS_SWIM;
    }
    item->fall_speed -= LARA_UW_FRICTION;
    CLAMPL(item->fall_speed, 0);
    if (item->fall_speed <= LARA_MAX_SWIM_SPEED * 2 / 3) {
        item->goal_anim_state = LS_TREAD;
    }
}

void __cdecl Lara_State_Tread(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_UW_DEATH;
        return;
    }

    if (g_Input & IN_ROLL) {
        item->current_anim_state = LS_WATER_ROLL;
        item->anim_num = LA_UNDERWATER_ROLL_START;
        item->frame_num = g_Anims[item->anim_num].frame_base;
        return;
    }

    if (g_Input & IN_LOOK) {
        Lara_LookUpDown();
    }
    Lara_SwimTurn(item);
    if (g_Input & IN_JUMP) {
        item->goal_anim_state = LS_SWIM;
    }
    item->fall_speed -= LARA_UW_FRICTION;
    CLAMPL(item->fall_speed, 0);
    if (g_Lara.gun_status == LGS_HANDS_BUSY) {
        g_Lara.gun_status = LGS_ARMLESS;
    }
}

void __cdecl Lara_State_Dive(ITEM *item, COLL_INFO *coll)
{
    if (g_Input & IN_FORWARD) {
        item->rot.x -= PHD_DEGREE;
    }
}

void __cdecl Lara_State_UWDeath(ITEM *item, COLL_INFO *coll)
{
    item->gravity = 0;
    item->fall_speed -= 8;
    CLAMPL(item->fall_speed, 0);

    int32_t angle = 2 * PHD_DEGREE;
    if (item->rot.x >= -angle && item->rot.x <= angle) {
        item->rot.x = 0;
    } else if (item->rot.x >= 0) {
        item->rot.x -= angle;
    } else {
        item->rot.x += angle;
    }
}

void __cdecl Lara_State_UWTwist(ITEM *item, COLL_INFO *coll)
{
    item->fall_speed = 0;
}
