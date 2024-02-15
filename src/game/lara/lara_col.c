#include "game/lara/lara_col.h"

#include "config.h"
#include "game/collide.h"
#include "game/input.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/lara/lara_misc.h"
#include "game/room.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/vars.h"

#include <stddef.h>
#include <stdint.h>

#define LF_WALK_STEP_L_START 0
#define LF_WALK_STEP_L_NEAR_END 5
#define LF_WALK_STEP_L_END 6
#define LF_WALK_STEP_R_START 7
#define LF_WALK_STEP_R_MID 22
#define LF_WALK_STEP_R_NEAR_END 23
#define LF_WALK_STEP_R_END 25
#define LF_WALK_STEP_L_2_START 26
#define LF_WALK_STEP_L_2_END 35

#define LF_RUN_L_START 0
#define LF_RUN_L_HEEL_GROUND 3
#define LF_RUN_L_END 9
#define LF_RUN_R_START 10
#define LF_RUN_R_FOOT_GROUND 14
#define LF_RUN_R_END 21

#define LF_BACK_R_START 26
#define LF_BACK_R_END 55

void (*g_LaraCollisionRoutines[])(ITEM_INFO *item, COLL_INFO *coll) = {
    Lara_Col_Walk,        Lara_Col_Run,       Lara_Col_Stop,
    Lara_Col_ForwardJump, Lara_Col_Pose,      Lara_Col_FastBack,
    Lara_Col_TurnR,       Lara_Col_TurnL,     Lara_Col_Death,
    Lara_Col_FastFall,    Lara_Col_Hang,      Lara_Col_Reach,
    Lara_Col_Splat,       Lara_Col_Tread,     Lara_Col_Land,
    Lara_Col_Compress,    Lara_Col_Back,      Lara_Col_Swim,
    Lara_Col_Glide,       Lara_Col_Null,      Lara_Col_FastTurn,
    Lara_Col_StepRight,   Lara_Col_StepLeft,  Lara_Col_Roll2,
    Lara_Col_Slide,       Lara_Col_BackJump,  Lara_Col_RightJump,
    Lara_Col_LeftJump,    Lara_Col_UpJump,    Lara_Col_FallBack,
    Lara_Col_HangLeft,    Lara_Col_HangRight, Lara_Col_SlideBack,
    Lara_Col_SurfTread,   Lara_Col_SurfSwim,  Lara_Col_Dive,
    Lara_Col_PushBlock,   Lara_Col_PullBlock, Lara_Col_PPReady,
    Lara_Col_Pickup,      Lara_Col_SwitchOn,  Lara_Col_SwitchOff,
    Lara_Col_UseKey,      Lara_Col_UsePuzzle, Lara_Col_UWDeath,
    Lara_Col_Roll,        Lara_Col_Special,   Lara_Col_SurfBack,
    Lara_Col_SurfLeft,    Lara_Col_SurfRight, Lara_Col_UseMidas,
    Lara_Col_DieMidas,    Lara_Col_SwanDive,  Lara_Col_FastDive,
    Lara_Col_Gymnast,     Lara_Col_WaterOut,  Lara_Col_Controlled,
    Lara_Col_Twist,       Lara_Col_UWRoll,
};

static void Lara_Col_Default(ITEM_INFO *item, COLL_INFO *coll);
static void Lara_Col_Jumper(ITEM_INFO *item, COLL_INFO *coll);

static void Lara_Col_Default(ITEM_INFO *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y;
    coll->bad_pos = STEPUP_HEIGHT;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_pits = 1;
    coll->slopes_are_walls = 1;
    Lara_GetCollisionInfo(item, coll);
}

static void Lara_Col_Jumper(ITEM_INFO *item, COLL_INFO *coll)
{
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = BAD_JUMP_CEILING;
    Lara_GetCollisionInfo(item, coll);

    Lara_DeflectEdgeJump(item, coll);

    if (item->fall_speed > 0 && coll->mid_floor <= 0) {
        if (Lara_LandedBad(item, coll)) {
            item->goal_anim_state = LS_DEATH;
        } else {
            item->goal_anim_state = LS_STOP;
        }
        item->pos.y += coll->mid_floor;
        item->gravity_status = 0;
        item->fall_speed = 0;
    }
}

void Lara_Col_Walk(ITEM_INFO *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y;
    item->gravity_status = 0;
    item->fall_speed = 0;
    coll->bad_pos = STEPUP_HEIGHT;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_pits = 1;
    coll->slopes_are_walls = 1;
    coll->lava_is_pit = 1;
    Lara_GetCollisionInfo(item, coll);

    if (Lara_HitCeiling(item, coll)) {
        return;
    }
    if (Lara_TestVault(item, coll)) {
        return;
    }

    if (Lara_DeflectEdge(item, coll)) {
        if (Item_TestAnimEqual(item, LA_WALK_FORWARD)
            && Item_TestFrameRange(
                item, LF_WALK_STEP_R_START, LF_WALK_STEP_R_END)) {
            Item_SwitchToAnim(item, LA_STOP_RIGHT, 0);
        } else if (
            Item_TestAnimEqual(item, LA_WALK_FORWARD)
            && (Item_TestFrameRange(
                    item, LF_WALK_STEP_L_START, LF_WALK_STEP_L_END)
                || Item_TestFrameRange(
                    item, LF_WALK_STEP_L_2_START, LF_WALK_STEP_L_2_END))) {
            Item_SwitchToAnim(item, LA_STOP_LEFT, 0);
        } else {
            Item_SwitchToAnim(item, LA_STOP, 0);
        }
    }

    if (Lara_Fallen(item, coll)) {
        return;
    }

    if (coll->mid_floor > STEP_L / 2) {
        if (Item_TestAnimEqual(item, LA_WALK_FORWARD)
            && Item_TestFrameRange(
                item, LF_WALK_STEP_L_END, LF_WALK_STEP_R_NEAR_END)) {
            Item_SwitchToAnim(item, LA_WALK_STEP_DOWN_RIGHT, 0);
        } else {
            Item_SwitchToAnim(item, LA_WALK_STEP_DOWN_LEFT, 0);
        }
    }

    if (coll->mid_floor >= -STEPUP_HEIGHT && coll->mid_floor < -STEP_L / 2) {
        if (Item_TestAnimEqual(item, LA_WALK_FORWARD)
            && Item_TestFrameRange(
                item, LF_WALK_STEP_L_NEAR_END, LF_WALK_STEP_R_MID)) {
            Item_SwitchToAnim(item, LA_WALK_STEP_UP_RIGHT, 0);
        } else {
            Item_SwitchToAnim(item, LA_WALK_STEP_UP_LEFT, 0);
        }
    }

    if (Lara_TestSlide(item, coll)) {
        return;
    }

    item->pos.y += coll->mid_floor;
}

void Lara_Col_Run(ITEM_INFO *item, COLL_INFO *coll)
{
    if (g_Config.fix_qwop_glitch) {
        item->gravity_status = 0;
        item->fall_speed = 0;
    }

    g_Lara.move_angle = item->rot.y;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_walls = 1;
    Lara_GetCollisionInfo(item, coll);

    if (Lara_HitCeiling(item, coll)) {
        return;
    }
    if (Lara_TestVault(item, coll)) {
        return;
    }

    if (Lara_DeflectEdge(item, coll)) {
        item->rot.z = 0;

        if (coll->front_type == HT_WALL
            && coll->front_floor < -(STEP_L * 5) / 2) {
            item->current_anim_state = LS_SPLAT;
            if (Item_TestAnimEqual(item, LA_RUN)
                && Item_TestFrameRange(item, LF_RUN_L_START, LF_RUN_L_END)) {
                Item_SwitchToAnim(item, LA_HIT_WALL_LEFT, 0);
                return;
            }
            if (Item_TestAnimEqual(item, LA_RUN)
                && Item_TestFrameRange(item, LF_RUN_R_START, LF_RUN_R_END)) {
                Item_SwitchToAnim(item, LA_HIT_WALL_RIGHT, 0);
                return;
            }
        }
        Item_SwitchToAnim(item, LA_STOP, 0);
    }

    if (Lara_Fallen(item, coll)) {
        return;
    }

    if (coll->mid_floor >= -STEPUP_HEIGHT && coll->mid_floor < -STEP_L / 2) {
        if (Item_TestAnimEqual(item, LA_RUN)
            && Item_TestFrameRange(
                item, LF_RUN_L_HEEL_GROUND, LF_RUN_R_FOOT_GROUND)) {
            Item_SwitchToAnim(item, LA_RUN_STEP_UP_LEFT, 0);
        } else {
            Item_SwitchToAnim(item, LA_RUN_STEP_UP_RIGHT, 0);
        }
    }

    if (Lara_TestSlide(item, coll)) {
        return;
    }

    if (coll->mid_floor >= 50) {
        item->pos.y += 50;
    } else {
        item->pos.y += coll->mid_floor;
    }
}

void Lara_Col_Stop(ITEM_INFO *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y;
    item->gravity_status = 0;
    item->fall_speed = 0;
    coll->bad_pos = STEPUP_HEIGHT;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_pits = 1;
    coll->slopes_are_walls = 1;
    Lara_GetCollisionInfo(item, coll);

    if (Lara_HitCeiling(item, coll)) {
        return;
    }

    if (g_Config.fix_descending_glitch && Lara_Fallen(item, coll)) {
        return;
    }

    if (coll->mid_floor > 100) {
        item->current_anim_state = LS_JUMP_FORWARD;
        item->goal_anim_state = LS_JUMP_FORWARD;
        Item_SwitchToAnim(item, LA_FALL_DOWN, 0);
        item->gravity_status = 1;
        item->fall_speed = 0;
        return;
    }

    if (Lara_TestSlide(item, coll)) {
        return;
    }

    Item_ShiftCol(item, coll);
    item->pos.y += coll->mid_floor;
}

void Lara_Col_ForwardJump(ITEM_INFO *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = BAD_JUMP_CEILING;
    Lara_GetCollisionInfo(item, coll);

    Lara_DeflectEdgeJump(item, coll);

    if (item->fall_speed > 0 && coll->mid_floor <= 0) {
        if (Lara_LandedBad(item, coll)) {
            item->goal_anim_state = LS_DEATH;
        } else if (g_Input.forward && !g_Input.slow) {
            item->goal_anim_state = LS_RUN;
        } else {
            item->goal_anim_state = LS_STOP;
        }
        item->pos.y += coll->mid_floor;
        item->gravity_status = 0;
        item->fall_speed = 0;
        item->speed = 0;

        if (!g_Config.fix_wall_jump_glitch) {
            Lara_Animate(item);
        }
    }
}

void Lara_Col_Pose(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara_Col_Stop(item, coll);
}

void Lara_Col_FastBack(ITEM_INFO *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y - PHD_180;
    item->gravity_status = 0;
    item->fall_speed = 0;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_pits = 1;
    coll->slopes_are_walls = 1;
    Lara_GetCollisionInfo(item, coll);

    if (Lara_HitCeiling(item, coll)) {
        return;
    }

    if (coll->mid_floor > 200) {
        item->current_anim_state = LS_FALL_BACK;
        item->goal_anim_state = LS_FALL_BACK;
        Item_SwitchToAnim(item, LA_FALL_BACK, 0);
        item->gravity_status = 1;
        item->fall_speed = 0;
        return;
    }

    if (Lara_DeflectEdge(item, coll)) {
        Item_SwitchToAnim(item, LA_STOP, 0);
    }

    item->pos.y += coll->mid_floor;
}

void Lara_Col_TurnR(ITEM_INFO *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y;
    item->gravity_status = 0;
    item->fall_speed = 0;
    coll->bad_pos = STEPUP_HEIGHT;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_pits = 1;
    coll->slopes_are_walls = 1;
    Lara_GetCollisionInfo(item, coll);

    if (coll->mid_floor > 100) {
        item->current_anim_state = LS_JUMP_FORWARD;
        item->goal_anim_state = LS_JUMP_FORWARD;
        Item_SwitchToAnim(item, LA_FALL_DOWN, 0);
        item->gravity_status = 1;
        item->fall_speed = 0;
        return;
    }

    if (Lara_TestSlide(item, coll)) {
        return;
    }

    item->pos.y += coll->mid_floor;
}

void Lara_Col_TurnL(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara_Col_TurnR(item, coll);
}

void Lara_Col_Death(ITEM_INFO *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y;
    coll->bad_pos = STEPUP_HEIGHT;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->radius = LARA_RAD * 4;
    Lara_GetCollisionInfo(item, coll);

    Item_ShiftCol(item, coll);
    item->pos.y += coll->mid_floor;
    item->hit_points = -1;
    g_Lara.air = -1;
}

void Lara_Col_FastFall(ITEM_INFO *item, COLL_INFO *coll)
{
    item->gravity_status = 1;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = BAD_JUMP_CEILING;
    Lara_GetCollisionInfo(item, coll);

    Lara_SlideEdgeJump(item, coll);
    if (coll->mid_floor <= 0) {
        if (Lara_LandedBad(item, coll)) {
            item->goal_anim_state = LS_DEATH;
        } else {
            item->goal_anim_state = LS_STOP;
            item->current_anim_state = LS_STOP;
            Item_SwitchToAnim(item, LA_LAND_FAR, 0);
        }
        Sound_StopEffect(SFX_LARA_FALL, NULL);
        item->pos.y += coll->mid_floor;
        item->gravity_status = 0;
        item->fall_speed = 0;
    }
}

void Lara_Col_Hang(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara_HangTest(item, coll);
    if (item->goal_anim_state == LS_HANG && g_Input.forward) {
        if (coll->front_floor > -850 && coll->front_floor < -650
            && coll->front_floor - coll->front_ceiling >= 0
            && coll->left_floor - coll->left_ceiling >= 0
            && coll->right_floor - coll->right_ceiling >= 0
            && !coll->hit_static) {
            item->goal_anim_state = g_Input.slow ? LS_GYMNAST : LS_NULL;
        }
    }
}

void Lara_Col_Reach(ITEM_INFO *item, COLL_INFO *coll)
{
    item->gravity_status = 1;
    g_Lara.move_angle = item->rot.y;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = 0;
    coll->bad_ceiling = BAD_JUMP_CEILING;
    Lara_GetCollisionInfo(item, coll);

    if (Lara_TestHangJump(item, coll)) {
        return;
    }
    Lara_SlideEdgeJump(item, coll);

    if (item->fall_speed > 0 && coll->mid_floor <= 0) {
        if (Lara_LandedBad(item, coll)) {
            item->goal_anim_state = LS_DEATH;
        } else {
            item->goal_anim_state = LS_STOP;
        }
        item->pos.y += coll->mid_floor;
        item->gravity_status = 0;
        item->fall_speed = 0;
    }
}

void Lara_Col_Splat(ITEM_INFO *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y;
    coll->bad_pos = STEPUP_HEIGHT;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_walls = 1;
    coll->slopes_are_pits = 1;
    Lara_GetCollisionInfo(item, coll);
    Item_ShiftCol(item, coll);
}

void Lara_Col_Land(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara_Col_Stop(item, coll);
}

void Lara_Col_Compress(ITEM_INFO *item, COLL_INFO *coll)
{
    item->gravity_status = 0;
    item->fall_speed = 0;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = NO_BAD_NEG;
    coll->bad_ceiling = 0;
    Lara_GetCollisionInfo(item, coll);

    if (coll->mid_ceiling > -100) {
        item->goal_anim_state = LS_STOP;
        item->current_anim_state = LS_STOP;
        Item_SwitchToAnim(item, LA_STOP, 0);
        item->gravity_status = 0;
        item->fall_speed = 0;
        item->speed = 0;
        item->pos.x = coll->old.x;
        item->pos.y = coll->old.y;
        item->pos.z = coll->old.z;
    }
}

void Lara_Col_Back(ITEM_INFO *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y - PHD_180;
    item->gravity_status = 0;
    item->fall_speed = 0;
    coll->bad_pos = STEPUP_HEIGHT;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_walls = 1;
    coll->slopes_are_pits = 1;
    Lara_GetCollisionInfo(item, coll);

    if (Lara_HitCeiling(item, coll)) {
        return;
    }

    if (Lara_DeflectEdge(item, coll)) {
        Item_SwitchToAnim(item, LA_STOP, 0);
    }

    if (g_Config.fix_descending_glitch && Lara_Fallen(item, coll)) {
        return;
    }

    if (coll->mid_floor > STEP_L / 2 && coll->mid_floor < (STEP_L * 3) / 2) {
        if (Item_TestAnimEqual(item, LA_WALK_BACK)
            && Item_TestFrameRange(item, LF_BACK_R_START, LF_BACK_R_END)) {
            Item_SwitchToAnim(item, LA_BACK_STEP_DOWN_RIGHT, 0);
        } else {
            Item_SwitchToAnim(item, LA_BACK_STEP_DOWN_LEFT, 0);
        }
    }

    if (Lara_TestSlide(item, coll)) {
        return;
    }

    item->pos.y += coll->mid_floor;
}

void Lara_Col_Null(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara_Col_Default(item, coll);
}

void Lara_Col_FastTurn(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara_Col_Stop(item, coll);
}

void Lara_Col_StepRight(ITEM_INFO *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y + PHD_90;
    item->gravity_status = 0;
    item->fall_speed = 0;
    coll->bad_pos = STEP_L / 2;
    coll->bad_neg = -STEP_L / 2;
    coll->bad_ceiling = 0;
    coll->slopes_are_walls = 1;
    coll->slopes_are_pits = 1;
    Lara_GetCollisionInfo(item, coll);

    if (Lara_HitCeiling(item, coll)) {
        return;
    }

    if (Lara_DeflectEdge(item, coll)) {
        Item_SwitchToAnim(item, LA_STOP, 0);
    }

    if (g_Config.fix_descending_glitch && Lara_Fallen(item, coll)) {
        return;
    }

    if (Lara_TestSlide(item, coll)) {
        return;
    }

    item->pos.y += coll->mid_floor;
}

void Lara_Col_StepLeft(ITEM_INFO *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y - PHD_90;
    item->gravity_status = 0;
    item->fall_speed = 0;
    coll->bad_pos = 128;
    coll->bad_neg = -128;
    coll->bad_ceiling = 0;
    coll->slopes_are_walls = 1;
    coll->slopes_are_pits = 1;
    Lara_GetCollisionInfo(item, coll);

    if (Lara_HitCeiling(item, coll)) {
        return;
    }

    if (Lara_DeflectEdge(item, coll)) {
        Item_SwitchToAnim(item, LA_STOP, 0);
    }

    if (g_Config.fix_descending_glitch && Lara_Fallen(item, coll)) {
        return;
    }

    if (Lara_TestSlide(item, coll)) {
        return;
    }

    item->pos.y += coll->mid_floor;
}

void Lara_Col_Slide(ITEM_INFO *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y;
    Lara_SlideSlope(item, coll);
}

void Lara_Col_BackJump(ITEM_INFO *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y - PHD_180;
    Lara_Col_Jumper(item, coll);
}

void Lara_Col_RightJump(ITEM_INFO *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y + PHD_90;
    Lara_Col_Jumper(item, coll);
}

void Lara_Col_LeftJump(ITEM_INFO *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y - PHD_90;
    Lara_Col_Jumper(item, coll);
}

void Lara_Col_UpJump(ITEM_INFO *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = BAD_JUMP_CEILING;
    coll->facing = g_Lara.move_angle;
    if (g_Config.enable_lean_jumping && item->speed < 0) {
        coll->facing += PHD_180;
    }

    Collide_GetCollisionInfo(
        coll, item->pos.x, item->pos.y, item->pos.z, item->room_number, 870);

    if (Lara_TestHangJumpUp(item, coll)) {
        return;
    }

    Lara_SlideEdgeJump(item, coll);

    if (g_Config.enable_lean_jumping) {
        if (coll->coll_type != COLL_NONE) {
            item->speed = item->speed > 0 ? 2 : -2;
        } else if (item->fall_speed < -70) {
            if (g_Input.forward && item->speed < 5) {
                item->speed++;
            } else if (g_Input.back && item->speed > -5) {
                item->speed -= 2;
            }
        }
    }

    if (item->fall_speed <= 0 || coll->mid_floor > 0) {
        return;
    }

    if (Lara_LandedBad(item, coll)) {
        item->goal_anim_state = LS_DEATH;
    } else {
        item->goal_anim_state = LS_STOP;
    }
    item->pos.y += coll->mid_floor;
    item->gravity_status = 0;
    item->fall_speed = 0;
}

void Lara_Col_FallBack(ITEM_INFO *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y - PHD_180;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = BAD_JUMP_CEILING;
    Lara_GetCollisionInfo(item, coll);

    Lara_DeflectEdgeJump(item, coll);

    if (item->fall_speed > 0 && coll->mid_floor <= 0) {
        if (Lara_LandedBad(item, coll)) {
            item->goal_anim_state = LS_DEATH;
        } else {
            item->goal_anim_state = LS_STOP;
        }
        item->pos.y += coll->mid_floor;
        item->gravity_status = 0;
        item->fall_speed = 0;
    }
}

void Lara_Col_HangLeft(ITEM_INFO *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y - PHD_90;
    Lara_HangTest(item, coll);
    g_Lara.move_angle = item->rot.y - PHD_90;
}

void Lara_Col_HangRight(ITEM_INFO *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y + PHD_90;
    Lara_HangTest(item, coll);
    g_Lara.move_angle = item->rot.y + PHD_90;
}

void Lara_Col_SlideBack(ITEM_INFO *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y - PHD_180;
    Lara_SlideSlope(item, coll);
}

void Lara_Col_PushBlock(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara_Col_Default(item, coll);
}

void Lara_Col_PullBlock(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara_Col_Default(item, coll);
}

void Lara_Col_PPReady(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara_Col_Default(item, coll);
}

void Lara_Col_Pickup(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara_Col_Default(item, coll);
}

void Lara_Col_Controlled(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara_Col_Default(item, coll);
}

void Lara_Col_Twist(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara_Col_Default(item, coll);
}

void Lara_Col_UWRoll(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara_SwimCollision(item, coll);
}

void Lara_Col_SwitchOn(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara_Col_Default(item, coll);
}

void Lara_Col_SwitchOff(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara_Col_Default(item, coll);
}

void Lara_Col_UseKey(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara_Col_Default(item, coll);
}

void Lara_Col_UsePuzzle(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara_Col_Default(item, coll);
}

void Lara_Col_Roll(ITEM_INFO *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y;
    item->gravity_status = 0;
    item->fall_speed = 0;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_walls = 1;
    Lara_GetCollisionInfo(item, coll);

    if (Lara_HitCeiling(item, coll)) {
        return;
    }

    if (Lara_TestSlide(item, coll)) {
        return;
    }

    if (coll->mid_floor > 200) {
        item->current_anim_state = LS_JUMP_FORWARD;
        item->goal_anim_state = LS_JUMP_FORWARD;
        Item_SwitchToAnim(item, LA_FALL_DOWN, 0);
        item->gravity_status = 1;
        item->fall_speed = 0;
        return;
    }

    Item_ShiftCol(item, coll);
    item->pos.y += coll->mid_floor;
}

void Lara_Col_Roll2(ITEM_INFO *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y - PHD_180;
    item->gravity_status = 0;
    item->fall_speed = 0;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_walls = 1;
    Lara_GetCollisionInfo(item, coll);

    if (Lara_HitCeiling(item, coll)) {
        return;
    }

    if (Lara_TestSlide(item, coll)) {
        return;
    }

    if (coll->mid_floor > 200) {
        item->current_anim_state = LS_FALL_BACK;
        item->goal_anim_state = LS_FALL_BACK;
        Item_SwitchToAnim(item, LA_FALL_BACK, 0);
        item->gravity_status = 1;
        item->fall_speed = 0;
        return;
    }

    Item_ShiftCol(item, coll);
    item->pos.y += coll->mid_floor;
}

void Lara_Col_Special(ITEM_INFO *item, COLL_INFO *coll)
{
}

void Lara_Col_UseMidas(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara_Col_Default(item, coll);
}

void Lara_Col_DieMidas(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara_Col_Default(item, coll);
}

void Lara_Col_SwanDive(ITEM_INFO *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = BAD_JUMP_CEILING;
    Lara_GetCollisionInfo(item, coll);

    Lara_DeflectEdgeJump(item, coll);

    if (item->fall_speed > 0 && coll->mid_floor <= 0) {
        item->goal_anim_state = LS_STOP;
        item->gravity_status = 0;
        item->fall_speed = 0;
        item->pos.y += coll->mid_floor;
    }
}

void Lara_Col_FastDive(ITEM_INFO *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = BAD_JUMP_CEILING;
    Lara_GetCollisionInfo(item, coll);

    Lara_DeflectEdgeJump(item, coll);

    if (item->fall_speed > 0 && coll->mid_floor <= 0) {
        if (item->fall_speed > 133) {
            item->goal_anim_state = LS_DEATH;
        } else {
            item->goal_anim_state = LS_STOP;
        }
        item->gravity_status = 0;
        item->fall_speed = 0;
        item->pos.y += coll->mid_floor;
    }
}

void Lara_Col_Gymnast(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara_Col_Default(item, coll);
}

void Lara_Col_WaterOut(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara_Col_Default(item, coll);
}

void Lara_Col_SurfSwim(ITEM_INFO *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y;
    Lara_SurfaceCollision(item, coll);
}

void Lara_Col_SurfTread(ITEM_INFO *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y;
    Lara_SurfaceCollision(item, coll);
}

void Lara_Col_SurfBack(ITEM_INFO *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y - PHD_180;
    Lara_SurfaceCollision(item, coll);
}

void Lara_Col_SurfLeft(ITEM_INFO *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y - PHD_90;
    Lara_SurfaceCollision(item, coll);
}

void Lara_Col_SurfRight(ITEM_INFO *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y + PHD_90;
    Lara_SurfaceCollision(item, coll);
}

void Lara_Col_Swim(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara_SwimCollision(item, coll);
}

void Lara_Col_Glide(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara_SwimCollision(item, coll);
}

void Lara_Col_Tread(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara_SwimCollision(item, coll);
}

void Lara_Col_Dive(ITEM_INFO *item, COLL_INFO *coll)
{
    Lara_SwimCollision(item, coll);
}

void Lara_Col_UWDeath(ITEM_INFO *item, COLL_INFO *coll)
{
    item->hit_points = -1;
    g_Lara.air = -1;
    g_Lara.gun_status = LGS_HANDS_BUSY;
    int16_t wh = Room_GetWaterHeight(
        item->pos.x, item->pos.y, item->pos.z, item->room_number);
    if (wh != NO_HEIGHT && wh < item->pos.y - 100) {
        item->pos.y -= 5;
    }
    Lara_SwimCollision(item, coll);
}
