#include "game/lara/state.h"

#include "game/camera.h"
#include "game/input.h"
#include "game/lara/common.h"
#include "game/lara/look.h"
#include "game/objects/common.h"
#include "game/objects/effects/twinkle.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/lara/misc.h>
#include <libtrx/game/math.h>

#include <stdint.h>

#define LF_ROLL 2
#define LF_JUMP_READY 3

static bool m_JumpPermitted = true;
static bool m_HasResponsiveJumping = false;
static bool m_HasResponsiveSwimming = false;

static bool M_HasResponsiveState(LARA_ANIMATION anim_idx);

static bool M_HasResponsiveState(const LARA_ANIMATION anim_idx)
{
    const OBJECT *const obj = Object_Get(O_LARA);
    if (!obj->loaded) {
        return false;
    }

    const ANIM *const anim = Object_GetAnim(obj, anim_idx);
    for (int32_t i = 0; i < anim->num_changes; i++) {
        const ANIM_CHANGE *const change = Anim_GetChange(anim->change_idx + i);
        if (change->goal_anim_state == LS_RESPONSIVE) {
            return true;
        }
    }

    return false;
}

void Lara_State_Initialise(void)
{
    m_HasResponsiveJumping = M_HasResponsiveState(LA_RUN);
    m_HasResponsiveSwimming = M_HasResponsiveState(LA_SWIM_FORWARD);
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
        Item_SwitchToAnim(item, LA_ROLL, LF_ROLL);
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
        item->goal_anim_state =
            g_Config.gameplay.enable_tr2_jumping && m_HasResponsiveJumping
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
        Item_SwitchToAnim(item, LA_ROLL, LF_ROLL);
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
        if (item->fall_speed > LARA_FASTFALL_SPEED) {
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

void Lara_State_TurnR(ITEM *item, COLL_INFO *coll)
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

void Lara_State_TurnL(ITEM *item, COLL_INFO *coll)
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
    g_Camera.target_angle = CAM_A_HANG;
    g_Camera.target_elevation = CAM_E_HANG;
    if (g_Input.left || g_Input.step_left) {
        item->goal_anim_state = LS_HANG_LEFT;
    } else if (g_Input.right || g_Input.step_right) {
        item->goal_anim_state = LS_HANG_RIGHT;
    }
}

void Lara_State_Reach(ITEM *item, COLL_INFO *coll)
{
    g_Camera.target_angle = 85 * DEG_1;
    if (item->fall_speed > LARA_FASTFALL_SPEED) {
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

    if (item->fall_speed > LARA_FASTFALL_SPEED) {
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
        ? LS_BACK
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
    g_Camera.target_elevation = -45 * DEG_1;
    if (g_Input.jump
        && (!g_Config.gameplay.enable_jump_twists || !g_Input.back)) {
        item->goal_anim_state = LS_JUMP_FORWARD;
    }
}

void Lara_State_BackJump(ITEM *item, COLL_INFO *coll)
{
    g_Camera.target_angle = DEG_1 * 135;
    if (item->fall_speed > LARA_FASTFALL_SPEED) {
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
    if (item->fall_speed > LARA_FASTFALL_SPEED) {
        item->goal_anim_state = LS_FAST_FALL;
    }
}

void Lara_State_LeftJump(ITEM *item, COLL_INFO *coll)
{
    if (item->fall_speed > LARA_FASTFALL_SPEED) {
        item->goal_anim_state = LS_FAST_FALL;
    }
}

void Lara_State_UpJump(ITEM *item, COLL_INFO *coll)
{
    if (item->fall_speed
        > (g_Config.gameplay.enable_swing_cancel ? LARA_SWING_FASTFALL_SPEED
                                                 : LARA_FASTFALL_SPEED)) {
        item->goal_anim_state = LS_FAST_FALL;
    }
}

void Lara_State_FallBack(ITEM *item, COLL_INFO *coll)
{
    if (item->fall_speed > LARA_FASTFALL_SPEED) {
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
    g_Camera.target_angle = CAM_A_HANG;
    g_Camera.target_elevation = CAM_E_HANG;
    if (!g_Input.left && !g_Input.step_left) {
        item->goal_anim_state = LS_HANG;
    }
}

void Lara_State_HangRight(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_A_HANG;
    g_Camera.target_elevation = CAM_E_HANG;
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
    g_Camera.target_angle = 35 * DEG_1;
    g_Camera.target_elevation = -25 * DEG_1;
}

void Lara_State_PullBlock(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.flags = CF_FOLLOW_CENTRE;
    g_Camera.target_angle = 35 * DEG_1;
    g_Camera.target_elevation = -25 * DEG_1;
}

void Lara_State_PPReady(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = 75 * DEG_1;
    if (!g_Input.action) {
        item->goal_anim_state = LS_STOP;
    }
}

void Lara_State_Pickup(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = -130 * DEG_1;
    g_Camera.target_elevation = -15 * DEG_1;
    g_Camera.target_distance = WALL_L;
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
    g_Camera.target_angle = 80 * DEG_1;
    g_Camera.target_elevation = -25 * DEG_1;
    g_Camera.target_distance = WALL_L;
}

void Lara_State_SwitchOff(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = 80 * DEG_1;
    g_Camera.target_elevation = -25 * DEG_1;
    g_Camera.target_distance = WALL_L;
}

void Lara_State_UseKey(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = -80 * DEG_1;
    g_Camera.target_elevation = -25 * DEG_1;
    g_Camera.target_distance = WALL_L;
}

void Lara_State_UsePuzzle(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = -80 * DEG_1;
    g_Camera.target_elevation = -25 * DEG_1;
    g_Camera.target_distance = WALL_L;
}

void Lara_State_Special(ITEM *item, COLL_INFO *coll)
{
    ITEM *const target_item = Lara_GetDeathCameraTarget();
    if (target_item != nullptr) {
        g_Camera.item = target_item;
        g_Camera.flags = CF_CHASE_OBJECT;
        g_Camera.type = CAM_FIXED;
        g_Camera.target_angle = item->rot.y;
        g_Camera.target_distance = WALL_L * 2;
        g_Camera.target_elevation = -25 * DEG_1;
    } else {
        g_Camera.flags = CF_FOLLOW_CENTRE;
        g_Camera.target_angle = 170 * DEG_1;
        g_Camera.target_elevation = -25 * DEG_1;
    }
}

void Lara_State_UseMidas(ITEM *item, COLL_INFO *coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    Twinkle_SparkleItem(item, (1 << LM_HAND_L) | (1 << LM_HAND_R));
}

void Lara_State_DieMidas(ITEM *item, COLL_INFO *coll)
{
    item->gravity = 0;
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
        Object_SetReflective(O_HAIR, true);
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
    if (item->fall_speed > LARA_FASTFALL_SPEED
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

void Lara_State_UWRoll(ITEM *item, COLL_INFO *coll)
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
    if (item->fall_speed > SURF_MAXSPEED) {
        item->fall_speed = SURF_MAXSPEED;
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
            item->rot.y -= LARA_SLOW_TURN / 2;
        } else if (g_Input.right) {
            item->rot.y += LARA_SLOW_TURN / 2;
        }
    }

    if (!g_Input.back) {
        item->goal_anim_state = LS_SURF_TREAD;
    }

    item->fall_speed += 8;
    if (item->fall_speed > SURF_MAXSPEED) {
        item->fall_speed = SURF_MAXSPEED;
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
        if (item->fall_speed > SURF_MAXSPEED) {
            item->fall_speed = SURF_MAXSPEED;
        }
        return;
    }

    if (g_Input.left) {
        item->rot.y -= LARA_SLOW_TURN / 2;
    } else if (g_Input.right) {
        item->rot.y += LARA_SLOW_TURN / 2;
    }

    if (!g_Input.step_left) {
        item->goal_anim_state = LS_SURF_TREAD;
    }

    item->fall_speed += 8;
    if (item->fall_speed > SURF_MAXSPEED) {
        item->fall_speed = SURF_MAXSPEED;
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
        if (item->fall_speed > SURF_MAXSPEED) {
            item->fall_speed = SURF_MAXSPEED;
        }
        return;
    }

    if (g_Input.left) {
        item->rot.y -= LARA_SLOW_TURN / 2;
    } else if (g_Input.right) {
        item->rot.y += LARA_SLOW_TURN / 2;
    }

    if (!g_Input.step_right) {
        item->goal_anim_state = LS_SURF_TREAD;
    }

    item->fall_speed += 8;
    if (item->fall_speed > SURF_MAXSPEED) {
        item->fall_speed = SURF_MAXSPEED;
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
        if (g_Lara.dive_timer == DIVE_WAIT) {
            item->goal_anim_state = LS_SWIM;
            item->current_anim_state = LS_DIVE;
            Item_SwitchToAnim(item, LA_SURF_DIVE, 0);
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
        item->goal_anim_state = LS_UW_ROLL;
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
        if (item->fall_speed > UW_MAXSPEED * 2) {
            item->fall_speed = UW_MAXSPEED * 2;
        }
    } else if (item->fall_speed > UW_MAXSPEED) {
        item->fall_speed = UW_MAXSPEED;
    }

    if (!g_Input.jump) {
        item->goal_anim_state =
            g_Config.gameplay.enable_tr2_swim_cancel && m_HasResponsiveSwimming
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
        item->goal_anim_state = LS_UW_ROLL;
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

    item->fall_speed -= WATER_FRICTION;
    if (item->fall_speed < 0) {
        item->fall_speed = 0;
    }

    if (item->fall_speed <= (UW_MAXSPEED * 2) / 3) {
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
        item->goal_anim_state = LS_UW_ROLL;
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

    item->fall_speed -= WATER_FRICTION;
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
