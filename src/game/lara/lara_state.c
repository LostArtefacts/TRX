#include "game/lara/lara_state.h"

#include "config.h"
#include "game/input.h"
#include "game/items.h"
#include "game/lara/lara_look.h"
#include "game/objects/effects/twinkle.h"
#include "game/room.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/vars.h"
#include "math/math.h"

#include <stdbool.h>
#include <stdint.h>

void (*g_LaraStateRoutines[])(ITEM_INFO *item, COLL_INFO *coll) = {
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
    Lara_State_Twist,
};

static bool m_JumpPermitted = true;

static int16_t Lara_FloorFront(ITEM_INFO *item, PHD_ANGLE ang, int32_t dist);

static int16_t Lara_FloorFront(ITEM_INFO *item, PHD_ANGLE ang, int32_t dist)
{
    int32_t x = item->pos.x + ((Math_Sin(ang) * dist) >> W2V_SHIFT);
    int32_t y = item->pos.y - LARA_HEIGHT;
    int32_t z = item->pos.z + ((Math_Cos(ang) * dist) >> W2V_SHIFT);
    int16_t room_num = item->room_number;
    FLOOR_INFO *floor = Room_GetFloor(x, y, z, &room_num);
    int32_t height = Room_GetHeight(floor, x, y, z);
    if (height != NO_HEIGHT) {
        height -= item->pos.y;
    }
    return height;
}

void Lara_State_Walk(ITEM_INFO *item, COLL_INFO *coll)
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
    } else {
        item->goal_anim_state = LS_STOP;
    }
}

void Lara_State_Run(ITEM_INFO *item, COLL_INFO *coll)
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
        item->pos.z_rot -= LARA_LEAN_RATE;
        if (item->pos.z_rot < -LARA_LEAN_MAX) {
            item->pos.z_rot = -LARA_LEAN_MAX;
        }
    } else if (g_Input.right) {
        g_Lara.turn_rate += LARA_TURN_RATE;
        if (g_Lara.turn_rate > LARA_FAST_TURN) {
            g_Lara.turn_rate = LARA_FAST_TURN;
        }
        item->pos.z_rot += LARA_LEAN_RATE;
        if (item->pos.z_rot > LARA_LEAN_MAX) {
            item->pos.z_rot = LARA_LEAN_MAX;
        }
    }

    if (g_Config.enable_tr2_jumping) {
        int16_t anim =
            item->anim_number - g_Objects[item->object_number].anim_index;
        if (anim == LA_RUN_START) {
            m_JumpPermitted = false;
        } else if (anim != LA_RUN || item->frame_number == LF_JUMP_READY) {
            m_JumpPermitted = true;
        }
    }

    if (g_Input.jump && m_JumpPermitted && !item->gravity_status) {
        item->goal_anim_state = LS_JUMP_FORWARD;
    } else if (g_Input.forward) {
        item->goal_anim_state = g_Input.slow ? LS_WALK : LS_RUN;
    } else {
        item->goal_anim_state = LS_STOP;
    }
}

void Lara_State_Stop(ITEM_INFO *item, COLL_INFO *coll)
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

void Lara_State_ForwardJump(ITEM_INFO *item, COLL_INFO *coll)
{
    if (item->goal_anim_state == LS_SWAN_DIVE
        || item->goal_anim_state == LS_REACH) {
        item->goal_anim_state = LS_JUMP_FORWARD;
    }
    if (item->goal_anim_state != LS_DEATH && item->goal_anim_state != LS_STOP) {
        if (g_Input.action && g_Lara.gun_status == LGS_ARMLESS) {
            item->goal_anim_state = LS_REACH;
        }
        if (g_Config.enable_jump_twists) {
            if (g_Input.roll && item->goal_anim_state != LS_RUN) {
                item->goal_anim_state = LS_TWIST;
            } else if (
                item->goal_anim_state == LS_TWIST && !item->gravity_status) {
                item->goal_anim_state = LS_STOP;
            }
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

void Lara_State_Pose(ITEM_INFO *item, COLL_INFO *coll)
{
}

void Lara_State_FastBack(ITEM_INFO *item, COLL_INFO *coll)
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

void Lara_State_TurnR(ITEM_INFO *item, COLL_INFO *coll)
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

void Lara_State_TurnL(ITEM_INFO *item, COLL_INFO *coll)
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

void Lara_State_Death(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
}

void Lara_State_FastFall(ITEM_INFO *item, COLL_INFO *coll)
{
    item->speed = (item->speed * 95) / 100;
    if (item->fall_speed >= DAMAGE_START + DAMAGE_LENGTH) {
        Sound_Effect(SFX_LARA_FALL, &item->pos, SPM_NORMAL);
    }
}

void Lara_State_Hang(ITEM_INFO *item, COLL_INFO *coll)
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

void Lara_State_Reach(ITEM_INFO *item, COLL_INFO *coll)
{
    g_Camera.target_angle = 85 * PHD_DEGREE;
    if (item->fall_speed > LARA_FASTFALL_SPEED) {
        item->goal_anim_state = LS_FAST_FALL;
    }
}

void Lara_State_Splat(ITEM_INFO *item, COLL_INFO *coll)
{
}

void Lara_State_Land(ITEM_INFO *item, COLL_INFO *coll)
{
}

void Lara_State_Compress(ITEM_INFO *item, COLL_INFO *coll)
{
    if (g_Input.forward
        && Lara_FloorFront(item, item->pos.y_rot, 256) >= -STEPUP_HEIGHT) {
        item->goal_anim_state = LS_JUMP_FORWARD;
        g_Lara.move_angle = item->pos.y_rot;
    } else if (
        g_Input.left
        && Lara_FloorFront(item, item->pos.y_rot - PHD_90, 256)
            >= -STEPUP_HEIGHT) {
        item->goal_anim_state = LS_JUMP_LEFT;
        g_Lara.move_angle = item->pos.y_rot - PHD_90;
    } else if (
        g_Input.right
        && Lara_FloorFront(item, item->pos.y_rot + PHD_90, 256)
            >= -STEPUP_HEIGHT) {
        item->goal_anim_state = LS_JUMP_RIGHT;
        g_Lara.move_angle = item->pos.y_rot + PHD_90;
    } else if (
        g_Input.back
        && Lara_FloorFront(item, item->pos.y_rot - PHD_180, 256)
            >= -STEPUP_HEIGHT) {
        item->goal_anim_state = LS_JUMP_BACK;
        g_Lara.move_angle = item->pos.y_rot - PHD_180;
    }

    if (item->fall_speed > LARA_FASTFALL_SPEED) {
        item->goal_anim_state = LS_FAST_FALL;
    }
}

void Lara_State_Back(ITEM_INFO *item, COLL_INFO *coll)
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

void Lara_State_FastTurn(ITEM_INFO *item, COLL_INFO *coll)
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

void Lara_State_StepRight(ITEM_INFO *item, COLL_INFO *coll)
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

void Lara_State_StepLeft(ITEM_INFO *item, COLL_INFO *coll)
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

void Lara_State_Slide(ITEM_INFO *item, COLL_INFO *coll)
{
    g_Camera.flags = NO_CHUNKY;
    g_Camera.target_elevation = -45 * PHD_DEGREE;
    if (g_Input.jump) {
        item->goal_anim_state = LS_JUMP_FORWARD;
    }
}

void Lara_State_BackJump(ITEM_INFO *item, COLL_INFO *coll)
{
    g_Camera.target_angle = PHD_DEGREE * 135;
    if (item->fall_speed > LARA_FASTFALL_SPEED) {
        item->goal_anim_state = LS_FAST_FALL;
    }

    if (g_Config.enable_jump_twists) {
        if (g_Input.roll && item->goal_anim_state != LS_STOP) {
            item->goal_anim_state = LS_TWIST;
        } else if (item->goal_anim_state == LS_TWIST && !item->gravity_status) {
            item->goal_anim_state = LS_STOP;
        }
    }
}

void Lara_State_RightJump(ITEM_INFO *item, COLL_INFO *coll)
{
    if (item->fall_speed > LARA_FASTFALL_SPEED) {
        item->goal_anim_state = LS_FAST_FALL;
    }
}

void Lara_State_LeftJump(ITEM_INFO *item, COLL_INFO *coll)
{
    if (item->fall_speed > LARA_FASTFALL_SPEED) {
        item->goal_anim_state = LS_FAST_FALL;
    }
}

void Lara_State_UpJump(ITEM_INFO *item, COLL_INFO *coll)
{
    if (item->fall_speed > LARA_FASTFALL_SPEED) {
        item->goal_anim_state = LS_FAST_FALL;
    }
}

void Lara_State_FallBack(ITEM_INFO *item, COLL_INFO *coll)
{
    if (item->fall_speed > LARA_FASTFALL_SPEED) {
        item->goal_anim_state = LS_FAST_FALL;
    }
    if (g_Input.action && g_Lara.gun_status == LGS_ARMLESS) {
        item->goal_anim_state = LS_REACH;
    }
}

void Lara_State_HangLeft(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_A_HANG;
    g_Camera.target_elevation = CAM_E_HANG;
    if (!g_Input.left && !g_Input.step_left) {
        item->goal_anim_state = LS_HANG;
    }
}

void Lara_State_HangRight(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = CAM_A_HANG;
    g_Camera.target_elevation = CAM_E_HANG;
    if (!g_Input.right && !g_Input.step_right) {
        item->goal_anim_state = LS_HANG;
    }
}

void Lara_State_SlideBack(ITEM_INFO *item, COLL_INFO *coll)
{
    if (g_Input.jump) {
        item->goal_anim_state = LS_JUMP_BACK;
    }
}

void Lara_State_PushBlock(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    g_Camera.flags = FOLLOW_CENTRE;
    g_Camera.target_angle = 35 * PHD_DEGREE;
    g_Camera.target_elevation = -25 * PHD_DEGREE;
}

void Lara_State_PullBlock(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    g_Camera.flags = FOLLOW_CENTRE;
    g_Camera.target_angle = 35 * PHD_DEGREE;
    g_Camera.target_elevation = -25 * PHD_DEGREE;
}

void Lara_State_PPReady(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = 75 * PHD_DEGREE;
    if (!g_Input.action) {
        item->goal_anim_state = LS_STOP;
    }
}

void Lara_State_Pickup(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = -130 * PHD_DEGREE;
    g_Camera.target_elevation = -15 * PHD_DEGREE;
    g_Camera.target_distance = WALL_L;
}

void Lara_State_Controlled(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;

    if (item->frame_number == g_Anims[item->anim_number].frame_end - 1) {
        g_Lara.gun_status = LGS_ARMLESS;
    }
}

void Lara_State_SwitchOn(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = 80 * PHD_DEGREE;
    g_Camera.target_elevation = -25 * PHD_DEGREE;
    g_Camera.target_distance = WALL_L;
}

void Lara_State_SwitchOff(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = 80 * PHD_DEGREE;
    g_Camera.target_elevation = -25 * PHD_DEGREE;
    g_Camera.target_distance = WALL_L;
}

void Lara_State_UseKey(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = -80 * PHD_DEGREE;
    g_Camera.target_elevation = -25 * PHD_DEGREE;
    g_Camera.target_distance = WALL_L;
}

void Lara_State_UsePuzzle(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    g_Camera.target_angle = -80 * PHD_DEGREE;
    g_Camera.target_elevation = -25 * PHD_DEGREE;
    g_Camera.target_distance = WALL_L;
}

void Lara_State_Roll(ITEM_INFO *item, COLL_INFO *coll)
{
}

void Lara_State_Roll2(ITEM_INFO *item, COLL_INFO *coll)
{
}

void Lara_State_Special(ITEM_INFO *item, COLL_INFO *coll)
{
    g_Camera.flags = FOLLOW_CENTRE;
    g_Camera.target_angle = 170 * PHD_DEGREE;
    g_Camera.target_elevation = -25 * PHD_DEGREE;
}

void Lara_State_UseMidas(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    Twinkle_SparkleItem(item, (1 << LM_HAND_L) | (1 << LM_HAND_R));
}

void Lara_State_DieMidas(ITEM_INFO *item, COLL_INFO *coll)
{
    item->gravity_status = 0;
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;

    int frm = item->frame_number - g_Anims[item->anim_number].frame_base;
    switch (frm) {
    case 5:
        g_Lara.mesh_effects |= (1 << LM_FOOT_L);
        g_Lara.mesh_effects |= (1 << LM_FOOT_R);
        g_Lara.mesh_ptrs[LM_FOOT_L] =
            g_Meshes[g_Objects[O_LARA_EXTRA].mesh_index + LM_FOOT_L];
        g_Lara.mesh_ptrs[LM_FOOT_R] =
            g_Meshes[g_Objects[O_LARA_EXTRA].mesh_index + LM_FOOT_R];
        break;

    case 70:
        g_Lara.mesh_effects |= (1 << LM_CALF_L);
        g_Lara.mesh_ptrs[LM_CALF_L] =
            g_Meshes[(&g_Objects[O_LARA_EXTRA])->mesh_index + LM_CALF_L];
        break;

    case 90:
        g_Lara.mesh_effects |= (1 << LM_THIGH_L);
        g_Lara.mesh_ptrs[LM_THIGH_L] =
            g_Meshes[(&g_Objects[O_LARA_EXTRA])->mesh_index + LM_THIGH_L];
        break;

    case 100:
        g_Lara.mesh_effects |= (1 << LM_CALF_R);
        g_Lara.mesh_ptrs[LM_CALF_R] =
            g_Meshes[(&g_Objects[O_LARA_EXTRA])->mesh_index + LM_CALF_R];
        break;

    case 120:
        g_Lara.mesh_effects |= (1 << LM_HIPS);
        g_Lara.mesh_effects |= (1 << LM_THIGH_R);
        g_Lara.mesh_ptrs[LM_HIPS] =
            g_Meshes[(&g_Objects[O_LARA_EXTRA])->mesh_index + LM_HIPS];
        g_Lara.mesh_ptrs[LM_THIGH_R] =
            g_Meshes[(&g_Objects[O_LARA_EXTRA])->mesh_index + LM_THIGH_R];
        break;

    case 135:
        g_Lara.mesh_effects |= (1 << LM_TORSO);
        g_Lara.mesh_ptrs[LM_TORSO] =
            g_Meshes[(&g_Objects[O_LARA_EXTRA])->mesh_index + LM_TORSO];
        break;

    case 150:
        g_Lara.mesh_effects |= (1 << LM_UARM_L);
        g_Lara.mesh_ptrs[LM_UARM_L] =
            g_Meshes[(&g_Objects[O_LARA_EXTRA])->mesh_index + LM_UARM_L];
        break;

    case 163:
        g_Lara.mesh_effects |= (1 << LM_LARM_L);
        g_Lara.mesh_ptrs[LM_LARM_L] =
            g_Meshes[(&g_Objects[O_LARA_EXTRA])->mesh_index + LM_LARM_L];
        break;

    case 174:
        g_Lara.mesh_effects |= (1 << LM_HAND_L);
        g_Lara.mesh_ptrs[LM_HAND_L] =
            g_Meshes[(&g_Objects[O_LARA_EXTRA])->mesh_index + LM_HAND_L];
        break;

    case 186:
        g_Lara.mesh_effects |= (1 << LM_UARM_R);
        g_Lara.mesh_ptrs[LM_UARM_R] =
            g_Meshes[(&g_Objects[O_LARA_EXTRA])->mesh_index + LM_UARM_R];
        break;

    case 195:
        g_Lara.mesh_effects |= (1 << LM_LARM_R);
        g_Lara.mesh_ptrs[LM_LARM_R] =
            g_Meshes[(&g_Objects[O_LARA_EXTRA])->mesh_index + LM_LARM_R];
        break;

    case 218:
        g_Lara.mesh_effects |= (1 << LM_HAND_R);
        g_Lara.mesh_ptrs[LM_HAND_R] =
            g_Meshes[(&g_Objects[O_LARA_EXTRA])->mesh_index + LM_HAND_R];
        break;

    case 225:
        g_Lara.mesh_effects |= (1 << LM_HEAD);
        g_Lara.mesh_ptrs[LM_HEAD] =
            g_Meshes[(&g_Objects[O_LARA_EXTRA])->mesh_index + LM_HEAD];
        break;
    }

    Twinkle_SparkleItem(item, g_Lara.mesh_effects);
}

void Lara_State_SwanDive(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 1;
    if (item->fall_speed > LARA_FASTFALL_SPEED
        && item->goal_anim_state != LS_DIVE) {
        item->goal_anim_state = LS_FAST_DIVE;
    }
}

void Lara_State_FastDive(ITEM_INFO *item, COLL_INFO *coll)
{
    if (g_Config.enable_jump_twists && g_Input.roll
        && item->goal_anim_state == LS_FAST_DIVE) {
        item->goal_anim_state = LS_TWIST;
    }

    coll->enable_spaz = 0;
    coll->enable_baddie_push = 1;
    item->speed = (item->speed * 95) / 100;
}

void Lara_State_Twist(ITEM_INFO *item, COLL_INFO *coll)
{
}

void Lara_State_Null(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
}

void Lara_State_Gymnast(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
}

void Lara_State_WaterOut(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    coll->enable_baddie_push = 0;
    g_Camera.flags = FOLLOW_CENTRE;
}

void Lara_State_SurfSwim(ITEM_INFO *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_UW_DEATH;
        return;
    }

    coll->enable_spaz = 0;
    g_Lara.dive_timer = 0;

    if (!g_Config.enable_tr3_sidesteps || !g_Input.slow) {
        if (g_Input.left) {
            item->pos.y_rot -= LARA_SLOW_TURN;
        } else if (g_Input.right) {
            item->pos.y_rot += LARA_SLOW_TURN;
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

void Lara_State_SurfBack(ITEM_INFO *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_UW_DEATH;
        return;
    }

    coll->enable_spaz = 0;
    g_Lara.dive_timer = 0;

    if (!g_Config.enable_tr3_sidesteps || !g_Input.slow) {
        if (g_Input.left) {
            item->pos.y_rot -= LARA_SLOW_TURN / 2;
        } else if (g_Input.right) {
            item->pos.y_rot += LARA_SLOW_TURN / 2;
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

void Lara_State_SurfLeft(ITEM_INFO *item, COLL_INFO *coll)
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
        item->pos.y_rot -= LARA_SLOW_TURN / 2;
    } else if (g_Input.right) {
        item->pos.y_rot += LARA_SLOW_TURN / 2;
    }

    if (!g_Input.step_left) {
        item->goal_anim_state = LS_SURF_TREAD;
    }

    item->fall_speed += 8;
    if (item->fall_speed > SURF_MAXSPEED) {
        item->fall_speed = SURF_MAXSPEED;
    }
}

void Lara_State_SurfRight(ITEM_INFO *item, COLL_INFO *coll)
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
        item->pos.y_rot -= LARA_SLOW_TURN / 2;
    } else if (g_Input.right) {
        item->pos.y_rot += LARA_SLOW_TURN / 2;
    }

    if (!g_Input.step_right) {
        item->goal_anim_state = LS_SURF_TREAD;
    }

    item->fall_speed += 8;
    if (item->fall_speed > SURF_MAXSPEED) {
        item->fall_speed = SURF_MAXSPEED;
    }
}

void Lara_State_SurfTread(ITEM_INFO *item, COLL_INFO *coll)
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
        item->pos.y_rot -= LARA_SLOW_TURN;
    } else if (g_Input.right) {
        item->pos.y_rot += LARA_SLOW_TURN;
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
            Item_SwitchToAnim(item, LA_SURF_DIVE, -1);
            item->pos.x_rot = -45 * PHD_DEGREE;
            item->fall_speed = 80;
            g_Lara.water_status = LWS_UNDERWATER;
        }
    } else {
        g_Lara.dive_timer = 0;
    }
}

void Lara_State_Swim(ITEM_INFO *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_UW_DEATH;
        return;
    }

    coll->enable_spaz = 0;

    if (g_Input.forward) {
        item->pos.x_rot -= 2 * PHD_DEGREE;
    }
    if (g_Input.back) {
        item->pos.x_rot += 2 * PHD_DEGREE;
    }
    if (g_Input.left) {
        item->pos.y_rot -= LARA_MED_TURN;
        item->pos.z_rot -= LARA_LEAN_RATE * 2;
    } else if (g_Input.right) {
        item->pos.y_rot += LARA_MED_TURN;
        item->pos.z_rot += LARA_LEAN_RATE * 2;
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

void Lara_State_Glide(ITEM_INFO *item, COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_UW_DEATH;
        return;
    }

    coll->enable_spaz = 0;

    if (g_Input.forward) {
        item->pos.x_rot -= 2 * PHD_DEGREE;
    } else if (g_Input.back) {
        item->pos.x_rot += 2 * PHD_DEGREE;
    }
    if (g_Input.left) {
        item->pos.y_rot -= LARA_MED_TURN;
        item->pos.z_rot -= LARA_LEAN_RATE * 2;
    } else if (g_Input.right) {
        item->pos.y_rot += LARA_MED_TURN;
        item->pos.z_rot += LARA_LEAN_RATE * 2;
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

void Lara_State_Tread(ITEM_INFO *item, COLL_INFO *coll)
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

    if (g_Input.forward) {
        item->pos.x_rot -= 2 * PHD_DEGREE;
    } else if (g_Input.back) {
        item->pos.x_rot += 2 * PHD_DEGREE;
    }
    if (g_Input.left) {
        item->pos.y_rot -= LARA_MED_TURN;
        item->pos.z_rot -= LARA_LEAN_RATE * 2;
    } else if (g_Input.right) {
        item->pos.y_rot += LARA_MED_TURN;
        item->pos.z_rot += LARA_LEAN_RATE * 2;
    }
    if (g_Input.jump) {
        item->goal_anim_state = LS_SWIM;
    }

    item->fall_speed -= WATER_FRICTION;
    if (item->fall_speed < 0) {
        item->fall_speed = 0;
    }
}

void Lara_State_Dive(ITEM_INFO *item, COLL_INFO *coll)
{
    if (g_Input.forward) {
        item->pos.x_rot -= PHD_DEGREE;
    }
}

void Lara_State_UWDeath(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->enable_spaz = 0;
    item->fall_speed -= 8;
    if (item->fall_speed <= 0) {
        item->fall_speed = 0;
    }

    if (item->pos.x_rot >= -2 * PHD_DEGREE
        && item->pos.x_rot <= 2 * PHD_DEGREE) {
        item->pos.x_rot = 0;
    } else if (item->pos.x_rot < 0) {
        item->pos.x_rot += 2 * PHD_DEGREE;
    } else {
        item->pos.x_rot -= 2 * PHD_DEGREE;
    }
}
