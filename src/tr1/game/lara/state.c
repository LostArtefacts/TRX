#include "game/lara/state.h"

#include "config.h"
#include "game/input.h"
#include "game/items.h"
#include "game/lara/common.h"
#include "game/lara/look.h"
#include "game/objects/common.h"
#include "game/objects/effects/twinkle.h"
#include "game/room.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/vars.h"
#include "math/math.h"

#include <stdbool.h>
#include <stdint.h>

#define LF_ROLL 2
#define LF_JUMP_READY 3

void (*g_LaraStateRoutines[])(ITEM *item, COLL_INFO *coll) = {
    Lara_State_Walk,        Lara_State_Run,       Lara_State_Stop,
    Lara_State_ForwardJump, Lara_State_Pose,      Lara_State_FastBack,
    Lara_State_TurnR,       Lara_State_TurnL,     Lara_State_Death,
    Lara_State_FastFall,    Lara_State_Hang,      Lara_State_Reach,
    Lara_State_Splat,       Lara_State_Tread,     Lara_State_Land,
    Lara_State_Compress,    Lara_State_Back,      Lara_State_Swim,
    Lara_State_Glide,       Lara_State_Null,      Lara_State_FastTurn,
    Lara_State_StepRight,   Lara_State_StepLeft,  Lara_State_Roll2,
    Lara_State_Slide,       Lara_State_BackJump,  Lara_State_RightJump,
    Lara_State_LeftJump,    Lara_State_UpJump,    Lara_State_FallBack,
    Lara_State_HangLeft,    Lara_State_HangRight, Lara_State_SlideBack,
    Lara_State_SurfTread,   Lara_State_SurfSwim,  Lara_State_Dive,
    Lara_State_PushBlock,   Lara_State_PullBlock, Lara_State_PPReady,
    Lara_State_Pickup,      Lara_State_SwitchOn,  Lara_State_SwitchOff,
    Lara_State_UseKey,      Lara_State_UsePuzzle, Lara_State_UWDeath,
    Lara_State_Roll,        Lara_State_Special,   Lara_State_SurfBack,
    Lara_State_SurfLeft,    Lara_State_SurfRight, Lara_State_UseMidas,
    Lara_State_DieMidas,    Lara_State_SwanDive,  Lara_State_FastDive,
    Lara_State_Gymnast,     Lara_State_WaterOut,  Lara_State_Controlled,
    Lara_State_Twist,       Lara_State_UWRoll,
};

static bool m_JumpPermitted = true;

static int16_t M_FloorFront(ITEM *item, PHD_ANGLE ang, int32_t dist);

static int16_t M_FloorFront(ITEM *item, PHD_ANGLE ang, int32_t dist)
{
    int32_t x = item->pos.x + ((Math_Sin(ang) * dist) >> W2V_SHIFT);
    int32_t y = item->pos.y - LARA_HEIGHT;
    int32_t z = item->pos.z + ((Math_Cos(ang) * dist) >> W2V_SHIFT);
    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(x, y, z, &room_num);
    int32_t height = Room_GetHeight(sector, x, y, z);
    if (height != NO_HEIGHT) {
        height -= item->pos.y;
    }
    return height;
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
        item->goal_anim_state = g_Input.slow ? LS_WALK : LS_RUN;
        if (g_Config.enable_tr2_jumping && !g_Input.slow) {
            m_JumpPermitted = true;
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

    if (g_Config.enable_tr2_jumping) {
        int16_t anim = item->anim_num - g_Objects[item->object_id].anim_idx;
        if (anim == LA_RUN_START) {
            m_JumpPermitted = false;
        } else if (
            anim != LA_RUN || Item_TestFrameEqual(item, LF_JUMP_READY - 1)) {
            m_JumpPermitted = true;
        }
    }

    if (g_Input.jump && m_JumpPermitted && !item->gravity) {
        item->goal_anim_state = LS_JUMP_FORWARD;
    } else if (g_Input.forward) {
        item->goal_anim_state = g_Input.slow ? LS_WALK : LS_RUN;
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

    if (g_Input.roll) {
        item->current_anim_state = LS_ROLL;
        item->goal_anim_state = LS_STOP;
        Item_SwitchToAnim(item, LA_ROLL, LF_ROLL);
        return;
    }

    item->goal_anim_state = LS_STOP;
    if (g_Input.look) {
        Lara_LookUpDown();
        if (!g_Config.enable_enhanced_look) {
            Lara_LookLeftRight();
            return;
        }
    }
    if (!g_Config.enable_enhanced_look && g_Camera.type == CAM_LOOK) {
        g_Camera.type = CAM_CHASE;
    }

    if (g_Input.step_left) {
        item->goal_anim_state = LS_STEP_LEFT;
    } else if (g_Input.step_right) {
        item->goal_anim_state = LS_STEP_RIGHT;
    }

    if (g_Input.left) {
        item->goal_anim_state = LS_TURN_L;
    } else if (g_Input.right) {
        item->goal_anim_state = LS_TURN_R;
    }

    if (g_Input.jump) {
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
        if (g_Config.enable_jump_twists && (g_Input.roll || g_Input.back)) {
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

void Lara_State_Pose(ITEM *item, COLL_INFO *coll)
{
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

    if (g_Config.enable_enhanced_look && g_Input.look) {
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
        item->goal_anim_state = g_Input.slow ? LS_WALK : LS_RUN;
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

    if (g_Config.enable_enhanced_look && g_Input.look) {
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
        item->goal_anim_state = g_Input.slow ? LS_WALK : LS_RUN;
    } else if (!g_Input.left) {
        item->goal_anim_state = LS_STOP;
    }
}

void Lara_State_Death(ITEM *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
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
    if (g_Config.enable_enhanced_look && g_Input.look) {
        Lara_LookUpDown();
    }

    coll->enable_spaz = 0;
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
    g_Camera.target_angle = 85 * PHD_DEGREE;
    if (item->fall_speed > LARA_FASTFALL_SPEED) {
        item->goal_anim_state = LS_FAST_FALL;
    }
}

void Lara_State_Splat(ITEM *item, COLL_INFO *coll)
{
}

void Lara_State_Land(ITEM *item, COLL_INFO *coll)
{
}

void Lara_State_Compress(ITEM *item, COLL_INFO *coll)
{
    if (g_Input.forward
        && M_FloorFront(item, item->rot.y, 256) >= -STEPUP_HEIGHT) {
        item->goal_anim_state = LS_JUMP_FORWARD;
        g_Lara.move_angle = item->rot.y;
    } else if (
        g_Input.left
        && M_FloorFront(item, item->rot.y - PHD_90, 256) >= -STEPUP_HEIGHT) {
        item->goal_anim_state = LS_JUMP_LEFT;
        g_Lara.move_angle = item->rot.y - PHD_90;
    } else if (
        g_Input.right
        && M_FloorFront(item, item->rot.y + PHD_90, 256) >= -STEPUP_HEIGHT) {
        item->goal_anim_state = LS_JUMP_RIGHT;
        g_Lara.move_angle = item->rot.y + PHD_90;
    } else if (
        g_Input.back
        && M_FloorFront(item, item->rot.y - PHD_180, 256) >= -STEPUP_HEIGHT) {
        item->goal_anim_state = LS_JUMP_BACK;
        g_Lara.move_angle = item->rot.y - PHD_180;
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

    item->goal_anim_state = g_Input.back && g_Input.slow ? LS_BACK : LS_STOP;

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

    if (g_Config.enable_enhanced_look && g_Input.look) {
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
    g_Camera.flags = NO_CHUNKY;
    g_Camera.target_elevation = -45 * PHD_DEGREE;
    if (g_Input.jump && (!g_Config.enable_jump_twists || !g_Input.back)) {
        item->goal_anim_state = LS_JUMP_FORWARD;
    }
}

void Lara_State_BackJump(ITEM *item, COLL_INFO *coll)
{
    g_Camera.target_angle = PHD_DEGREE * 135;
    if (item->fall_speed > LARA_FASTFALL_SPEED) {
        item->goal_anim_state = LS_FAST_FALL;
    } else if (item->goal_anim_state == LS_RUN) {
        item->goal_anim_state = LS_STOP;
    } else if (
        item->goal_anim_state != LS_STOP && g_Config.enable_jump_twists
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
        > (g_Config.enable_swing_cancel ? LARA_SWING_FASTFALL_SPEED
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
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_A_HANG;
    g_Camera.target_elevation = CAM_E_HANG;
    if (!g_Input.left && !g_Input.step_left) {
        item->goal_anim_state = LS_HANG;
    }
}

void Lara_State_HangRight(ITEM *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_A_HANG;
    g_Camera.target_elevation = CAM_E_HANG;
    if (!g_Input.right && !g_Input.step_right) {
        item->goal_anim_state = LS_HANG;
    }
}

void Lara_State_SlideBack(ITEM *item, COLL_INFO *coll)
{
    if (g_Input.jump && (!g_Config.enable_jump_twists || !g_Input.forward)) {
        item->goal_anim_state = LS_JUMP_BACK;
    }
}

void Lara_State_PushBlock(ITEM *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    g_Camera.flags = FOLLOW_CENTRE;
    g_Camera.target_angle = 35 * PHD_DEGREE;
    g_Camera.target_elevation = -25 * PHD_DEGREE;
}

void Lara_State_PullBlock(ITEM *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    g_Camera.flags = FOLLOW_CENTRE;
    g_Camera.target_angle = 35 * PHD_DEGREE;
    g_Camera.target_elevation = -25 * PHD_DEGREE;
}

void Lara_State_PPReady(ITEM *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = 75 * PHD_DEGREE;
    if (!g_Input.action) {
        item->goal_anim_state = LS_STOP;
    }
}

void Lara_State_Pickup(ITEM *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = -130 * PHD_DEGREE;
    g_Camera.target_elevation = -15 * PHD_DEGREE;
    g_Camera.target_distance = WALL_L;
}

void Lara_State_Controlled(ITEM *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
}

void Lara_State_SwitchOn(ITEM *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = 80 * PHD_DEGREE;
    g_Camera.target_elevation = -25 * PHD_DEGREE;
    g_Camera.target_distance = WALL_L;
}

void Lara_State_SwitchOff(ITEM *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = 80 * PHD_DEGREE;
    g_Camera.target_elevation = -25 * PHD_DEGREE;
    g_Camera.target_distance = WALL_L;
}

void Lara_State_UseKey(ITEM *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = -80 * PHD_DEGREE;
    g_Camera.target_elevation = -25 * PHD_DEGREE;
    g_Camera.target_distance = WALL_L;
}

void Lara_State_UsePuzzle(ITEM *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = -80 * PHD_DEGREE;
    g_Camera.target_elevation = -25 * PHD_DEGREE;
    g_Camera.target_distance = WALL_L;
}

void Lara_State_Roll(ITEM *item, COLL_INFO *coll)
{
}

void Lara_State_Roll2(ITEM *item, COLL_INFO *coll)
{
}

void Lara_State_Special(ITEM *item, COLL_INFO *coll)
{
    g_Camera.flags = FOLLOW_CENTRE;
    g_Camera.target_angle = 170 * PHD_DEGREE;
    g_Camera.target_elevation = -25 * PHD_DEGREE;
}

void Lara_State_UseMidas(ITEM *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    Twinkle_SparkleItem(item, (1 << LM_HAND_L) | (1 << LM_HAND_R));
}

void Lara_State_DieMidas(ITEM *item, COLL_INFO *coll)
{
    item->gravity = 0;
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;

    Object_SetReflective(O_LARA_EXTRA, true);

    int frm = item->frame_num - g_Anims[item->anim_num].frame_base;
    switch (frm) {
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
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 1;
    if (item->fall_speed > LARA_FASTFALL_SPEED
        && item->goal_anim_state != LS_DIVE) {
        item->goal_anim_state = LS_FAST_DIVE;
    }
}

void Lara_State_FastDive(ITEM *item, COLL_INFO *coll)
{
    if (g_Config.enable_jump_twists && g_Input.roll
        && item->goal_anim_state == LS_FAST_DIVE) {
        item->goal_anim_state = LS_TWIST;
    }

    coll->enable_spaz = 0;
    coll->enable_baddie_push = 1;
    item->speed = (item->speed * 95) / 100;
}

void Lara_State_Twist(ITEM *item, COLL_INFO *coll)
{
}

void Lara_State_UWRoll(ITEM *item, COLL_INFO *coll)
{
    item->fall_speed = 0;
    item->goal_anim_state = LS_TREAD;
}

void Lara_State_Null(ITEM *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
}

void Lara_State_Gymnast(ITEM *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
}

void Lara_State_WaterOut(ITEM *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    g_Camera.flags = FOLLOW_CENTRE;
}

void Lara_State_SurfSwim(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_UW_DEATH;
        return;
    }

    coll->enable_spaz = 0;
    g_Lara.dive_timer = 0;

    if (!g_Config.enable_tr3_sidesteps || !g_Input.slow) {
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

    coll->enable_spaz = 0;
    g_Lara.dive_timer = 0;

    if (!g_Config.enable_tr3_sidesteps || !g_Input.slow) {
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

    coll->enable_spaz = 0;
    g_Lara.dive_timer = 0;

    if (g_Config.enable_tr3_sidesteps && g_Input.slow && g_Input.left) {
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

    coll->enable_spaz = 0;
    g_Lara.dive_timer = 0;

    if (g_Config.enable_tr3_sidesteps && g_Input.slow && g_Input.right) {
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

    coll->enable_spaz = 0;

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
        || (g_Config.enable_tr3_sidesteps && g_Input.slow && g_Input.left)) {
        item->goal_anim_state = LS_SURF_LEFT;
    } else if (
        g_Input.step_right
        || (g_Config.enable_tr3_sidesteps && g_Input.slow && g_Input.right)) {
        item->goal_anim_state = LS_SURF_RIGHT;
    }

    if (g_Input.jump) {
        g_Lara.dive_timer++;
        if (g_Lara.dive_timer == DIVE_WAIT) {
            item->goal_anim_state = LS_SWIM;
            item->current_anim_state = LS_DIVE;
            Item_SwitchToAnim(item, LA_SURF_DIVE, 0);
            item->rot.x = -45 * PHD_DEGREE;
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

    coll->enable_spaz = 0;

    if (g_Config.enable_uw_roll && g_Input.roll) {
        item->goal_anim_state = LS_UW_ROLL;
        return;
    }

    if (g_Input.forward) {
        item->rot.x -= 2 * PHD_DEGREE;
    }
    if (g_Input.back) {
        item->rot.x += 2 * PHD_DEGREE;
    }
    if (g_Config.enable_tr2_swimming) {
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
        item->goal_anim_state = LS_GLIDE;
    }
}

void Lara_State_Glide(ITEM *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_UW_DEATH;
        return;
    }

    coll->enable_spaz = 0;

    if (g_Config.enable_uw_roll && g_Input.roll) {
        item->goal_anim_state = LS_UW_ROLL;
        return;
    }

    if (g_Input.forward) {
        item->rot.x -= 2 * PHD_DEGREE;
    } else if (g_Input.back) {
        item->rot.x += 2 * PHD_DEGREE;
    }
    if (g_Config.enable_tr2_swimming) {
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
    if (g_Config.enable_enhanced_look) {
        if (g_Input.look) {
            Lara_LookUpDown();
        }
    }

    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_UW_DEATH;
        return;
    }

    coll->enable_spaz = 0;

    if (g_Config.enable_uw_roll && g_Input.roll) {
        item->goal_anim_state = LS_UW_ROLL;
        return;
    }

    if (g_Input.forward) {
        item->rot.x -= 2 * PHD_DEGREE;
    } else if (g_Input.back) {
        item->rot.x += 2 * PHD_DEGREE;
    }
    if (g_Config.enable_tr2_swimming) {
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
}

void Lara_State_Dive(ITEM *item, COLL_INFO *coll)
{
    if (g_Input.forward) {
        item->rot.x -= PHD_DEGREE;
    }
}

void Lara_State_UWDeath(ITEM *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    item->fall_speed -= 8;
    if (item->fall_speed <= 0) {
        item->fall_speed = 0;
    }

    if (item->rot.x >= -2 * PHD_DEGREE && item->rot.x <= 2 * PHD_DEGREE) {
        item->rot.x = 0;
    } else if (item->rot.x < 0) {
        item->rot.x += 2 * PHD_DEGREE;
    } else {
        item->rot.x -= 2 * PHD_DEGREE;
    }
}
