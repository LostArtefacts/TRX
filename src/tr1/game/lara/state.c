#include "game/lara/state.h"

#include "game/input.h"
#include "game/lara/common.h"
#include "game/lara/look.h"
#include "game/objects/common.h"
#include "game/objects/effects/twinkle.h"
#include "game/sound.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/lara/util.h>
#include <libtrx/game/math.h>

#include <stdint.h>

#define LF_ROLL 2
#define LF_JUMP_READY 3

static bool m_JumpPermitted = true;

void Lara_State_Walk(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_STOP;
        return;
    }

    if (g_Input.left) {
        g_Lara.turn_rate -= LARA_TURN_RATE;
        if (g_Lara.turn_rate < -LARA_SLOW_TURN) {
            g_Lara.turn_rate = -LARA_SLOW_TURN;
        }
    } else if (g_Input.right) {
        g_Lara.turn_rate += LARA_TURN_RATE;
        if (g_Lara.turn_rate > LARA_SLOW_TURN) {
            g_Lara.turn_rate = LARA_SLOW_TURN;
        }
    }

    if (g_Input.forward) {
        if (g_Lara.water_status == LWS_WADE) {
            item->goal_anim_state = LS_WADE;
        } else {
            item->goal_anim_state = g_Input.slow ? LS_WALK : LS_RUN;
            if (g_Config.gameplay.enable_tr2_jumping && !g_Input.slow) {
                m_JumpPermitted = true;
            }
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
        if (g_Lara.turn_rate < -LARA_FAST_TURN) {
            g_Lara.turn_rate = -LARA_FAST_TURN;
        }
        item->rot.z -= LARA_LEAN_RATE;
        if (item->rot.z < -LARA_LEAN_MAX) {
            item->rot.z = -LARA_LEAN_MAX;
        }
    } else if (g_Input.right) {
        g_Lara.turn_rate += LARA_TURN_RATE;
        if (g_Lara.turn_rate > LARA_FAST_TURN) {
            g_Lara.turn_rate = LARA_FAST_TURN;
        }
        item->rot.z += LARA_LEAN_RATE;
        if (item->rot.z > LARA_LEAN_MAX) {
            item->rot.z = LARA_LEAN_MAX;
        }
    }

    if (g_Config.gameplay.enable_tr2_jumping) {
        if (Item_TestAnimEqual(item, LA_RUN_START)) {
            m_JumpPermitted = false;
        } else if (
            !Item_TestAnimEqual(item, LA_RUN)
            || Item_TestFrameEqual(item, LF_JUMP_READY - 1)) {
            m_JumpPermitted = true;
        }
    } else {
        m_JumpPermitted = true;
    }

    if (g_Input.jump && m_JumpPermitted && !item->gravity) {
        item->goal_anim_state = g_Config.gameplay.enable_tr2_jumping
                && Lara_State_IsResponsive(LA_RUN)
            ? LS_RESPONSIVE
            : LS_JUMP_FORWARD;
    } else if (g_Input.forward) {
        if (g_Lara.water_status == LWS_WADE) {
            item->goal_anim_state = LS_WADE;
        } else {
            item->goal_anim_state = g_Input.slow ? LS_WALK : LS_RUN;
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

    if (g_Lara.interact_target.is_moving) {
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
        if (!g_Config.gameplay.enable_enhanced_look) {
            Lara_LookLeftRight();
            return;
        }
    }
    if (!g_Config.gameplay.enable_enhanced_look && g_Camera.type == CAM_LOOK) {
        g_Camera.type = CAM_CHASE;
    }

    if (g_Input.step_left) {
        item->goal_anim_state = LS_STEP_LEFT;
    } else if (g_Input.step_right) {
        item->goal_anim_state = LS_STEP_RIGHT;
    }

    if (g_Input.left) {
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
        if (g_Config.gameplay.enable_jump_twists
            && (g_Input.roll || g_Input.back)) {
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
        if (g_Lara.turn_rate < -LARA_JUMP_TURN) {
            g_Lara.turn_rate = -LARA_JUMP_TURN;
        }
    } else if (g_Input.right) {
        g_Lara.turn_rate += LARA_TURN_RATE;
        if (g_Lara.turn_rate > LARA_JUMP_TURN) {
            g_Lara.turn_rate = LARA_JUMP_TURN;
        }
    }
}

void Lara_State_FastBack(ITEM *item, COLL_INFO *coll)
{
    item->goal_anim_state = LS_STOP;
    if (g_Input.left) {
        g_Lara.turn_rate -= LARA_TURN_RATE;
        if (g_Lara.turn_rate < -LARA_MED_TURN) {
            g_Lara.turn_rate = -LARA_MED_TURN;
        }
    } else if (g_Input.right) {
        g_Lara.turn_rate += LARA_TURN_RATE;
        if (g_Lara.turn_rate > LARA_MED_TURN) {
            g_Lara.turn_rate = LARA_MED_TURN;
        }
    }
}

void Lara_State_TurnRight(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_STOP;
        return;
    }

    if (g_Config.gameplay.enable_enhanced_look && g_Input.look) {
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
        } else {
            item->goal_anim_state = g_Input.slow ? LS_WALK : LS_RUN;
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

    if (g_Config.gameplay.enable_enhanced_look && g_Input.look) {
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
        } else {
            item->goal_anim_state = g_Input.slow ? LS_WALK : LS_RUN;
        }
    } else if (!g_Input.left) {
        item->goal_anim_state = LS_STOP;
    }
}

void Lara_State_Death(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
}

void Lara_State_FastFall(ITEM *item, COLL_INFO *coll)
{
    item->speed = (item->speed * 95) / 100;
    if (item->fall_speed >= DAMAGE_START + DAMAGE_LENGTH) {
        Sound_Effect(SFX_LARA_FALL, &item->pos, SPM_NORMAL);
    }
}

void Lara_State_Hang(ITEM *item, COLL_INFO *coll)
{
    if (g_Config.gameplay.enable_enhanced_look && g_Input.look) {
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

void Lara_State_Reach(ITEM *item, COLL_INFO *coll)
{
    g_Camera.target_angle = CAM_REACH_ANGLE;
    if (item->fall_speed > LARA_FAST_FALL_SPEED) {
        item->goal_anim_state = LS_FAST_FALL;
    }
}

void Lara_State_Compress(ITEM *item, COLL_INFO *coll)
{
    if (g_Lara.water_status != LWS_WADE) {
        if (g_Input.forward
            && Lara_FloorFront(item, item->rot.y, 256) >= -STEPUP_HEIGHT) {
            item->goal_anim_state = LS_JUMP_FORWARD;
            g_Lara.move_angle = item->rot.y;
        } else if (
            g_Input.left
            && Lara_FloorFront(item, item->rot.y - DEG_90, 256)
                >= -STEPUP_HEIGHT) {
            item->goal_anim_state = LS_JUMP_LEFT;
            g_Lara.move_angle = item->rot.y - DEG_90;
        } else if (
            g_Input.right
            && Lara_FloorFront(item, item->rot.y + DEG_90, 256)
                >= -STEPUP_HEIGHT) {
            item->goal_anim_state = LS_JUMP_RIGHT;
            g_Lara.move_angle = item->rot.y + DEG_90;
        } else if (
            g_Input.back
            && Lara_FloorFront(item, item->rot.y - DEG_180, 256)
                >= -STEPUP_HEIGHT) {
            item->goal_anim_state = LS_JUMP_BACK;
            g_Lara.move_angle = item->rot.y - DEG_180;
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

    item->goal_anim_state =
        g_Input.back && (g_Input.slow || g_Lara.water_status == LWS_WADE)
        ? LS_WALK_BACK
        : LS_STOP;

    if (g_Input.left) {
        g_Lara.turn_rate -= LARA_TURN_RATE;
        if (g_Lara.turn_rate < -LARA_SLOW_TURN) {
            g_Lara.turn_rate = -LARA_SLOW_TURN;
        }
    } else if (g_Input.right) {
        g_Lara.turn_rate += LARA_TURN_RATE;
        if (g_Lara.turn_rate > LARA_SLOW_TURN) {
            g_Lara.turn_rate = LARA_SLOW_TURN;
        }
    }
}

void Lara_State_FastTurn(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_STOP;
        return;
    }

    if (g_Config.gameplay.enable_enhanced_look && g_Input.look) {
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
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_STOP;
        return;
    }

    if (!g_Input.step_right) {
        item->goal_anim_state = LS_STOP;
    }

    if (g_Input.left) {
        g_Lara.turn_rate -= LARA_TURN_RATE;
        if (g_Lara.turn_rate < -LARA_SLOW_TURN) {
            g_Lara.turn_rate = -LARA_SLOW_TURN;
        }
    } else if (g_Input.right) {
        g_Lara.turn_rate += LARA_TURN_RATE;
        if (g_Lara.turn_rate > LARA_SLOW_TURN) {
            g_Lara.turn_rate = LARA_SLOW_TURN;
        }
    }
}

void Lara_State_StepLeft(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_STOP;
        return;
    }

    if (!g_Input.step_left) {
        item->goal_anim_state = LS_STOP;
    }

    if (g_Input.left) {
        g_Lara.turn_rate -= LARA_TURN_RATE;
        if (g_Lara.turn_rate < -LARA_SLOW_TURN) {
            g_Lara.turn_rate = -LARA_SLOW_TURN;
        }
    } else if (g_Input.right) {
        g_Lara.turn_rate += LARA_TURN_RATE;
        if (g_Lara.turn_rate > LARA_SLOW_TURN) {
            g_Lara.turn_rate = LARA_SLOW_TURN;
        }
    }
}

void Lara_State_Slide(ITEM *item, COLL_INFO *coll)
{
    g_Camera.flags = CF_NO_CHUNKY;
    g_Camera.target_elevation = CAM_SLIDE_ELEVATION;
    if (g_Input.jump
        && (!g_Config.gameplay.enable_jump_twists || !g_Input.back)) {
        item->goal_anim_state = LS_JUMP_FORWARD;
    }
}

void Lara_State_BackJump(ITEM *item, COLL_INFO *coll)
{
    g_Camera.target_angle = CAM_BACK_JUMP_ANGLE;
    if (item->fall_speed > LARA_FAST_FALL_SPEED) {
        item->goal_anim_state = LS_FAST_FALL;
    } else if (item->goal_anim_state == LS_RUN) {
        item->goal_anim_state = LS_STOP;
    } else if (
        item->goal_anim_state != LS_STOP && g_Config.gameplay.enable_jump_twists
        && (g_Input.roll || g_Input.forward)) {
        item->goal_anim_state = LS_TWIST;
    }
}

void Lara_State_RightJump(ITEM *item, COLL_INFO *coll)
{
    if (item->fall_speed > LARA_FAST_FALL_SPEED) {
        item->goal_anim_state = LS_FAST_FALL;
    }
}

void Lara_State_LeftJump(ITEM *item, COLL_INFO *coll)
{
    if (item->fall_speed > LARA_FAST_FALL_SPEED) {
        item->goal_anim_state = LS_FAST_FALL;
    }
}

void Lara_State_UpJump(ITEM *item, COLL_INFO *coll)
{
    if (item->fall_speed
        > (g_Config.gameplay.enable_swing_cancel ? LARA_SWING_FAST_FALL_SPEED
                                                 : LARA_FAST_FALL_SPEED)) {
        item->goal_anim_state = LS_FAST_FALL;
    }
}

void Lara_State_FallBack(ITEM *item, COLL_INFO *coll)
{
    if (item->fall_speed > LARA_FAST_FALL_SPEED) {
        item->goal_anim_state = LS_FAST_FALL;
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
    if (g_Input.jump
        && (!g_Config.gameplay.enable_jump_twists || !g_Input.forward)) {
        item->goal_anim_state = LS_JUMP_BACK;
    }
}

void Lara_State_PushBlock(ITEM *item, COLL_INFO *coll)
{
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
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_PICKUP_ANGLE;
    g_Camera.target_elevation = CAM_PICKUP_ELEVATION;
    g_Camera.target_distance = CAM_PICKUP_DISTANCE;
}

void Lara_State_Controlled(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
}

void Lara_State_SwitchOn(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_SWITCH_ON_ANGLE;
    g_Camera.target_elevation = CAM_SWITCH_ON_ELEVATION;
    g_Camera.target_distance = CAM_SWITCH_ON_DISTANCE;
}

void Lara_State_UseKey(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_USE_KEY_ANGLE;
    g_Camera.target_elevation = CAM_USE_KEY_ELEVATION;
    g_Camera.target_distance = CAM_USE_KEY_DISTANCE;
}

void Lara_State_Special(ITEM *item, COLL_INFO *coll)
{
    ITEM *const target_item = Lara_GetDeathCameraTarget();
    if (target_item != nullptr) {
        g_Camera.item = target_item;
        g_Camera.flags = CF_CHASE_OBJECT;
        g_Camera.type = CAM_FIXED;
        g_Camera.target_angle = item->rot.y;
        g_Camera.target_distance = CAM_SPECIAL_DISTANCE;
    } else {
        g_Camera.flags = CF_FOLLOW_CENTRE;
        g_Camera.target_angle = CAM_SPECIAL_ANGLE;
    }
    g_Camera.target_elevation = CAM_SPECIAL_ELEVATION;
}

void Lara_State_UseMidas(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    Twinkle_SparkleItem(item, (1 << LM_HAND_L) | (1 << LM_HAND_R));
}

void Lara_State_DieMidas(ITEM *item, COLL_INFO *coll)
{
    item->gravity = false;
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;

    Object_SetReflective(O_LARA_EXTRA, true);

    const int32_t frame_num = Item_GetRelativeFrame(item);
    switch (frame_num) {
    case 5:
        g_Lara.mesh_effects |= (1 << LM_FOOT_L);
        g_Lara.mesh_effects |= (1 << LM_FOOT_R);
        Lara_SwapSingleMesh(LM_FOOT_L, O_LARA_EXTRA);
        Lara_SwapSingleMesh(LM_FOOT_R, O_LARA_EXTRA);
        break;

    case 70:
        g_Lara.mesh_effects |= (1 << LM_CALF_L);
        Lara_SwapSingleMesh(LM_CALF_L, O_LARA_EXTRA);
        break;

    case 90:
        g_Lara.mesh_effects |= (1 << LM_THIGH_L);
        Lara_SwapSingleMesh(LM_THIGH_L, O_LARA_EXTRA);
        break;

    case 100:
        g_Lara.mesh_effects |= (1 << LM_CALF_R);
        Lara_SwapSingleMesh(LM_CALF_R, O_LARA_EXTRA);
        break;

    case 120:
        g_Lara.mesh_effects |= (1 << LM_HIPS);
        g_Lara.mesh_effects |= (1 << LM_THIGH_R);
        Lara_SwapSingleMesh(LM_HIPS, O_LARA_EXTRA);
        Lara_SwapSingleMesh(LM_THIGH_R, O_LARA_EXTRA);
        break;

    case 135:
        g_Lara.mesh_effects |= (1 << LM_TORSO);
        Lara_SwapSingleMesh(LM_TORSO, O_LARA_EXTRA);
        break;

    case 150:
        g_Lara.mesh_effects |= (1 << LM_UARM_L);
        Lara_SwapSingleMesh(LM_UARM_L, O_LARA_EXTRA);
        break;

    case 163:
        g_Lara.mesh_effects |= (1 << LM_LARM_L);
        Lara_SwapSingleMesh(LM_LARM_L, O_LARA_EXTRA);
        break;

    case 174:
        g_Lara.mesh_effects |= (1 << LM_HAND_L);
        Lara_SwapSingleMesh(LM_HAND_L, O_LARA_EXTRA);
        break;

    case 186:
        g_Lara.mesh_effects |= (1 << LM_UARM_R);
        Lara_SwapSingleMesh(LM_UARM_R, O_LARA_EXTRA);
        break;

    case 195:
        g_Lara.mesh_effects |= (1 << LM_LARM_R);
        Lara_SwapSingleMesh(LM_LARM_R, O_LARA_EXTRA);
        break;

    case 218:
        g_Lara.mesh_effects |= (1 << LM_HAND_R);
        Lara_SwapSingleMesh(LM_HAND_R, O_LARA_EXTRA);
        break;

    case 225:
        Object_SetReflective(O_LARA_HAIR, true);
        g_Lara.mesh_effects |= (1 << LM_HEAD);
        Lara_SwapSingleMesh(LM_HEAD, O_LARA_EXTRA);
        break;
    }

    Twinkle_SparkleItem(item, g_Lara.mesh_effects);
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
    if (g_Config.gameplay.enable_jump_twists && g_Input.roll
        && item->goal_anim_state == LS_FAST_DIVE) {
        item->goal_anim_state = LS_TWIST;
    }

    coll->enable_hit = 0;
    coll->enable_baddie_push = 1;
    item->speed = (item->speed * 95) / 100;
}

void Lara_State_UWTwist(ITEM *item, COLL_INFO *coll)
{
    item->fall_speed = 0;
    item->goal_anim_state = LS_TREAD;
}

void Lara_State_Null(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
}

void Lara_State_WaterOut(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.flags = CF_FOLLOW_CENTRE;
}

void Lara_State_SurfSwim(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_UW_DEATH;
        return;
    }

    coll->enable_hit = 0;
    g_Lara.dive_timer = 0;

    if (!g_Config.input.enable_tr3_sidesteps || !g_Input.slow) {
        if (g_Input.left) {
            item->rot.y -= LARA_SLOW_TURN;
        } else if (g_Input.right) {
            item->rot.y += LARA_SLOW_TURN;
        }
    }

    if (!g_Input.forward) {
        item->goal_anim_state = LS_SURF_TREAD;
    }
    if (g_Input.jump) {
        item->goal_anim_state = LS_SURF_TREAD;
    }

    item->fall_speed += 8;
    if (item->fall_speed > LARA_MAX_SURF_SPEED) {
        item->fall_speed = LARA_MAX_SURF_SPEED;
    }
}

void Lara_State_SurfBack(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_UW_DEATH;
        return;
    }

    coll->enable_hit = 0;
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
    if (item->fall_speed > LARA_MAX_SURF_SPEED) {
        item->fall_speed = LARA_MAX_SURF_SPEED;
    }
}

void Lara_State_SurfLeft(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_UW_DEATH;
        return;
    }

    coll->enable_hit = 0;
    g_Lara.dive_timer = 0;

    if (g_Config.input.enable_tr3_sidesteps && g_Input.slow && g_Input.left) {
        item->fall_speed += 8;
        if (item->fall_speed > LARA_MAX_SURF_SPEED) {
            item->fall_speed = LARA_MAX_SURF_SPEED;
        }
        return;
    }

    if (g_Input.left) {
        item->rot.y -= LARA_SURF_TURN;
    } else if (g_Input.right) {
        item->rot.y += LARA_SURF_TURN;
    }

    if (!g_Input.step_left) {
        item->goal_anim_state = LS_SURF_TREAD;
    }

    item->fall_speed += 8;
    if (item->fall_speed > LARA_MAX_SURF_SPEED) {
        item->fall_speed = LARA_MAX_SURF_SPEED;
    }
}

void Lara_State_SurfRight(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_UW_DEATH;
        return;
    }

    coll->enable_hit = 0;
    g_Lara.dive_timer = 0;

    if (g_Config.input.enable_tr3_sidesteps && g_Input.slow && g_Input.right) {
        item->fall_speed += 8;
        if (item->fall_speed > LARA_MAX_SURF_SPEED) {
            item->fall_speed = LARA_MAX_SURF_SPEED;
        }
        return;
    }

    if (g_Input.left) {
        item->rot.y -= LARA_SURF_TURN;
    } else if (g_Input.right) {
        item->rot.y += LARA_SURF_TURN;
    }

    if (!g_Input.step_right) {
        item->goal_anim_state = LS_SURF_TREAD;
    }

    item->fall_speed += 8;
    if (item->fall_speed > LARA_MAX_SURF_SPEED) {
        item->fall_speed = LARA_MAX_SURF_SPEED;
    }
}

void Lara_State_SurfTread(ITEM *item, COLL_INFO *coll)
{
    item->fall_speed -= 4;
    if (item->fall_speed < 0) {
        item->fall_speed = 0;
    }

    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_UW_DEATH;
        return;
    }

    coll->enable_hit = 0;

    if (g_Input.look) {
        Lara_LookLeftRightSurf();
        Lara_LookUpDownSurf();
        return;
    }
    if (g_Camera.type == CAM_LOOK) {
        g_Camera.type = CAM_CHASE;
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

    if (g_Input.step_left
        || (g_Config.input.enable_tr3_sidesteps && g_Input.slow
            && g_Input.left)) {
        item->goal_anim_state = LS_SURF_LEFT;
    } else if (
        g_Input.step_right
        || (g_Config.input.enable_tr3_sidesteps && g_Input.slow
            && g_Input.right)) {
        item->goal_anim_state = LS_SURF_RIGHT;
    }

    if (g_Input.jump) {
        g_Lara.dive_timer++;
        if (g_Lara.dive_timer == LARA_DIVE_WAIT) {
            item->goal_anim_state = LS_SWIM;
            item->current_anim_state = LS_DIVE;
            Item_SwitchToAnim(item, LA_ONWATER_DIVE, 0);
            item->rot.x = -45 * DEG_1;
            item->fall_speed = 80;
            g_Lara.water_status = LWS_UNDERWATER;
        }
    } else {
        g_Lara.dive_timer = 0;
    }
}

void Lara_State_Swim(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_UW_DEATH;
        return;
    }

    coll->enable_hit = 0;

    if (g_Config.gameplay.enable_uw_roll && g_Input.roll) {
        item->goal_anim_state = LS_WATER_ROLL;
        return;
    }

    if (g_Input.forward) {
        item->rot.x -= 2 * DEG_1;
    }
    if (g_Input.back) {
        item->rot.x += 2 * DEG_1;
    }
    if (g_Config.gameplay.enable_tr2_swimming) {
        if (g_Input.left) {
            g_Lara.turn_rate -= LARA_TURN_RATE;
            CLAMPL(g_Lara.turn_rate, -LARA_MED_TURN);
            item->rot.z -= LARA_LEAN_RATE_SWIM;
        } else if (g_Input.right) {
            g_Lara.turn_rate += LARA_TURN_RATE;
            CLAMPG(g_Lara.turn_rate, LARA_MED_TURN);
            item->rot.z += LARA_LEAN_RATE_SWIM;
        }
    } else {
        if (g_Input.left) {
            item->rot.y -= LARA_MED_TURN;
            item->rot.z -= LARA_LEAN_RATE * 2;
        } else if (g_Input.right) {
            item->rot.y += LARA_MED_TURN;
            item->rot.z += LARA_LEAN_RATE * 2;
        }
    }

    item->fall_speed += 8;
    if (g_Lara.water_status == LWS_CHEAT) {
        if (item->fall_speed > LARA_MAX_SWIM_SPEED * 2) {
            item->fall_speed = LARA_MAX_SWIM_SPEED * 2;
        }
    } else if (item->fall_speed > LARA_MAX_SWIM_SPEED) {
        item->fall_speed = LARA_MAX_SWIM_SPEED;
    }

    if (!g_Input.jump) {
        item->goal_anim_state = g_Config.gameplay.enable_tr2_swim_cancel
                && Lara_State_IsResponsive(LA_UNDERWATER_SWIM_FORWARD)
            ? LS_RESPONSIVE
            : LS_GLIDE;
    }
}

void Lara_State_Glide(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_UW_DEATH;
        return;
    }

    coll->enable_hit = 0;

    if (g_Config.gameplay.enable_uw_roll && g_Input.roll) {
        item->goal_anim_state = LS_WATER_ROLL;
        return;
    }

    if (g_Input.forward) {
        item->rot.x -= 2 * DEG_1;
    } else if (g_Input.back) {
        item->rot.x += 2 * DEG_1;
    }
    if (g_Config.gameplay.enable_tr2_swimming) {
        if (g_Input.left) {
            g_Lara.turn_rate -= LARA_TURN_RATE;
            CLAMPL(g_Lara.turn_rate, -LARA_MED_TURN);
            item->rot.z -= LARA_LEAN_RATE_SWIM;
        } else if (g_Input.right) {
            g_Lara.turn_rate += LARA_TURN_RATE;
            CLAMPG(g_Lara.turn_rate, LARA_MED_TURN);
            item->rot.z += LARA_LEAN_RATE_SWIM;
        }
    } else {
        if (g_Input.left) {
            item->rot.y -= LARA_MED_TURN;
            item->rot.z -= LARA_LEAN_RATE * 2;
        } else if (g_Input.right) {
            item->rot.y += LARA_MED_TURN;
            item->rot.z += LARA_LEAN_RATE * 2;
        }
    }

    if (g_Input.jump) {
        item->goal_anim_state = LS_SWIM;
    }

    item->fall_speed -= LARA_UW_FRICTION;
    if (item->fall_speed < 0) {
        item->fall_speed = 0;
    }

    if (item->fall_speed <= (LARA_MAX_SWIM_SPEED * 2) / 3) {
        item->goal_anim_state = LS_TREAD;
    }
}

void Lara_State_Tread(ITEM *item, COLL_INFO *coll)
{
    if (g_Config.gameplay.enable_enhanced_look) {
        if (g_Input.look) {
            Lara_LookUpDown();
        }
    }

    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_UW_DEATH;
        return;
    }

    coll->enable_hit = 0;

    if (g_Config.gameplay.enable_uw_roll && g_Input.roll) {
        item->goal_anim_state = LS_WATER_ROLL;
        return;
    }

    if (g_Input.forward) {
        item->rot.x -= 2 * DEG_1;
    } else if (g_Input.back) {
        item->rot.x += 2 * DEG_1;
    }
    if (g_Config.gameplay.enable_tr2_swimming) {
        if (g_Input.left) {
            g_Lara.turn_rate -= LARA_TURN_RATE;
            CLAMPL(g_Lara.turn_rate, -LARA_MED_TURN);
            item->rot.z -= LARA_LEAN_RATE_SWIM;
        } else if (g_Input.right) {
            g_Lara.turn_rate += LARA_TURN_RATE;
            CLAMPG(g_Lara.turn_rate, LARA_MED_TURN);
            item->rot.z += LARA_LEAN_RATE_SWIM;
        }
    } else {
        if (g_Input.left) {
            item->rot.y -= LARA_MED_TURN;
            item->rot.z -= LARA_LEAN_RATE * 2;
        } else if (g_Input.right) {
            item->rot.y += LARA_MED_TURN;
            item->rot.z += LARA_LEAN_RATE * 2;
        }
    }

    if (g_Input.jump) {
        item->goal_anim_state = LS_SWIM;
    }

    item->fall_speed -= LARA_UW_FRICTION;
    if (item->fall_speed < 0) {
        item->fall_speed = 0;
    }
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
    coll->enable_hit = 0;
    item->fall_speed -= 8;
    if (item->fall_speed <= 0) {
        item->fall_speed = 0;
    }

    if (item->rot.x >= -2 * DEG_1 && item->rot.x <= 2 * DEG_1) {
        item->rot.x = 0;
    } else if (item->rot.x < 0) {
        item->rot.x += 2 * DEG_1;
    } else {
        item->rot.x -= 2 * DEG_1;
    }
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

// clang-format off
REGISTER_LARA_STATE(LS_WALK,          Lara_State_Walk)
REGISTER_LARA_STATE(LS_RUN,           Lara_State_Run)
REGISTER_LARA_STATE(LS_STOP,          Lara_State_Stop)
REGISTER_LARA_STATE(LS_JUMP_FORWARD,  Lara_State_ForwardJump)
REGISTER_LARA_STATE(LS_FAST_BACK,     Lara_State_FastBack)
REGISTER_LARA_STATE(LS_TURN_RIGHT,    Lara_State_TurnRight)
REGISTER_LARA_STATE(LS_TURN_LEFT,     Lara_State_TurnLeft)
REGISTER_LARA_STATE(LS_DEATH,         Lara_State_Death)
REGISTER_LARA_STATE(LS_FAST_FALL,     Lara_State_FastFall)
REGISTER_LARA_STATE(LS_HANG,          Lara_State_Hang)
REGISTER_LARA_STATE(LS_REACH,         Lara_State_Reach)
REGISTER_LARA_STATE(LS_TREAD,         Lara_State_Tread)
REGISTER_LARA_STATE(LS_COMPRESS,      Lara_State_Compress)
REGISTER_LARA_STATE(LS_WALK_BACK,     Lara_State_Back)
REGISTER_LARA_STATE(LS_SWIM,          Lara_State_Swim)
REGISTER_LARA_STATE(LS_GLIDE,         Lara_State_Glide)
REGISTER_LARA_STATE(LS_PULL_UP,       Lara_State_Null)
REGISTER_LARA_STATE(LS_FAST_TURN,     Lara_State_FastTurn)
REGISTER_LARA_STATE(LS_STEP_RIGHT,    Lara_State_StepRight)
REGISTER_LARA_STATE(LS_STEP_LEFT,     Lara_State_StepLeft)
REGISTER_LARA_STATE(LS_SLIDE,         Lara_State_Slide)
REGISTER_LARA_STATE(LS_JUMP_BACK,     Lara_State_BackJump)
REGISTER_LARA_STATE(LS_JUMP_RIGHT,    Lara_State_RightJump)
REGISTER_LARA_STATE(LS_JUMP_LEFT,     Lara_State_LeftJump)
REGISTER_LARA_STATE(LS_JUMP_UP,       Lara_State_UpJump)
REGISTER_LARA_STATE(LS_FALL_BACK,     Lara_State_FallBack)
REGISTER_LARA_STATE(LS_SHIMMY_LEFT,   Lara_State_HangLeft)
REGISTER_LARA_STATE(LS_SHIMMY_RIGHT,  Lara_State_HangRight)
REGISTER_LARA_STATE(LS_SLIDE_BACK,    Lara_State_SlideBack)
REGISTER_LARA_STATE(LS_SURF_TREAD,    Lara_State_SurfTread)
REGISTER_LARA_STATE(LS_SURF_SWIM,     Lara_State_SurfSwim)
REGISTER_LARA_STATE(LS_DIVE,          Lara_State_Dive)
REGISTER_LARA_STATE(LS_PUSH_BLOCK,    Lara_State_PushBlock)
REGISTER_LARA_STATE(LS_PULL_BLOCK,    Lara_State_PushBlock)
REGISTER_LARA_STATE(LS_PP_READY,      Lara_State_PPReady)
REGISTER_LARA_STATE(LS_PICKUP,        Lara_State_Pickup)
REGISTER_LARA_STATE(LS_SWITCH_ON,     Lara_State_SwitchOn)
REGISTER_LARA_STATE(LS_SWITCH_OFF,    Lara_State_SwitchOn)
REGISTER_LARA_STATE(LS_USE_KEY,       Lara_State_UseKey)
REGISTER_LARA_STATE(LS_USE_PUZZLE,    Lara_State_UseKey)
REGISTER_LARA_STATE(LS_UW_DEATH,      Lara_State_UWDeath)
REGISTER_LARA_STATE(LS_SPECIAL,       Lara_State_Special)
REGISTER_LARA_STATE(LS_SURF_BACK,     Lara_State_SurfBack)
REGISTER_LARA_STATE(LS_SURF_LEFT,     Lara_State_SurfLeft)
REGISTER_LARA_STATE(LS_SURF_RIGHT,    Lara_State_SurfRight)
REGISTER_LARA_STATE(LS_USE_MIDAS,     Lara_State_UseMidas)
REGISTER_LARA_STATE(LS_DIE_MIDAS,     Lara_State_DieMidas)
REGISTER_LARA_STATE(LS_SWAN_DIVE,     Lara_State_SwanDive)
REGISTER_LARA_STATE(LS_FAST_DIVE,     Lara_State_FastDive)
REGISTER_LARA_STATE(LS_GYMNAST,       Lara_State_Null)
REGISTER_LARA_STATE(LS_WATER_OUT,     Lara_State_WaterOut)
REGISTER_LARA_STATE(LS_CONTROLLED,    Lara_State_Controlled)
REGISTER_LARA_STATE(LS_WATER_ROLL,    Lara_State_UWTwist)
REGISTER_LARA_STATE(LS_WADE,          Lara_State_Wade)
// clang-format on
