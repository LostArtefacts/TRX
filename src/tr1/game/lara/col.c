#include "game/input.h"
#include "game/lara/common.h"
#include "game/lara/misc.h"
#include "game/sound.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/collision.h>
#include <libtrx/game/lara.h>

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

#define LF_WADE_L_START 0
#define LF_WADE_L_END 9
#define LF_WADE_R_START 10
#define LF_WADE_R_END 21

#define LF_WADE_STEP_L_START 3
#define LF_WADE_STEP_L_END 14

static void M_CollideStop(ITEM *item, const COLL_INFO *coll);

static void M_CollideStop(ITEM *const item, const COLL_INFO *const coll)
{
    switch (coll->old_anim_state) {
    case LS_STOP:
    case LS_TURN_RIGHT:
    case LS_TURN_LEFT:
    case LS_FAST_TURN:
        item->current_anim_state = coll->old_anim_state;
        item->anim_num = coll->old_anim_num;
        item->frame_num = coll->old_frame_num;
        if (g_Input.left) {
            item->goal_anim_state = LS_TURN_LEFT;
        } else if (g_Input.right) {
            item->goal_anim_state = LS_TURN_RIGHT;
        } else {
            item->goal_anim_state = LS_STOP;
        }
        Lara_Animate(item);
        break;

    default:
        Item_SwitchToAnim(item, LA_STAND_STILL, 0);
        break;
    }
}

void Lara_Col_Walk(ITEM *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y;
    item->gravity = false;
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
            Item_SwitchToAnim(item, LA_WALK_STOP_RIGHT, 0);
        } else if (
            Item_TestAnimEqual(item, LA_WALK_FORWARD)
            && (Item_TestFrameRange(
                    item, LF_WALK_STEP_L_START, LF_WALK_STEP_L_END)
                || Item_TestFrameRange(
                    item, LF_WALK_STEP_L_2_START, LF_WALK_STEP_L_2_END))) {
            Item_SwitchToAnim(item, LA_WALK_STOP_LEFT, 0);
        } else {
            Item_SwitchToAnim(item, LA_STAND_STILL, 0);
        }
    }

    if (Lara_Fallen(item, coll)) {
        return;
    }

    if (coll->side_mid.floor > STEP_L / 2) {
        if (Item_TestAnimEqual(item, LA_WALK_FORWARD)
            && Item_TestFrameRange(
                item, LF_WALK_STEP_L_END, LF_WALK_STEP_R_NEAR_END)) {
            Item_SwitchToAnim(item, LA_WALK_DOWN_LEFT, 0);
        } else {
            Item_SwitchToAnim(item, LA_WALK_DOWN_RIGHT, 0);
        }
    }

    if (coll->side_mid.floor >= -STEPUP_HEIGHT
        && coll->side_mid.floor < -STEP_L / 2) {
        if (Item_TestAnimEqual(item, LA_WALK_FORWARD)
            && Item_TestFrameRange(
                item, LF_WALK_STEP_L_NEAR_END, LF_WALK_STEP_R_MID)) {
            Item_SwitchToAnim(item, LA_WALK_UP_STEP_LEFT, 0);
        } else {
            Item_SwitchToAnim(item, LA_WALK_UP_STEP_RIGHT, 0);
        }
    }

    if (Lara_TestSlide(item, coll)) {
        return;
    }

    item->pos.y += coll->side_mid.floor;
}

void Lara_Col_Run(ITEM *item, COLL_INFO *coll)
{
    if (g_Config.gameplay.fix_qwop_glitch) {
        item->gravity = false;
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

        if (coll->side_front.type == HT_WALL
            && coll->side_front.floor < -(STEP_L * 5) / 2) {
            item->current_anim_state = LS_SPLAT;
            if (Item_TestAnimEqual(item, LA_RUN)
                && Item_TestFrameRange(item, LF_RUN_L_START, LF_RUN_L_END)) {
                Item_SwitchToAnim(item, LA_WALL_SMASH_LEFT, 0);
                return;
            }
            if (Item_TestAnimEqual(item, LA_RUN)
                && Item_TestFrameRange(item, LF_RUN_R_START, LF_RUN_R_END)) {
                Item_SwitchToAnim(item, LA_WALL_SMASH_RIGHT, 0);
                return;
            }
        }
        Item_SwitchToAnim(item, LA_STAND_STILL, 0);
    }

    if (Lara_Fallen(item, coll)) {
        return;
    }

    if (coll->side_mid.floor >= -STEPUP_HEIGHT
        && coll->side_mid.floor < -STEP_L / 2) {
        if (Item_TestAnimEqual(item, LA_RUN)
            && Item_TestFrameRange(
                item, LF_RUN_L_HEEL_GROUND, LF_RUN_R_FOOT_GROUND)) {
            Item_SwitchToAnim(item, LA_RUN_UP_STEP_LEFT, 0);
        } else {
            Item_SwitchToAnim(item, LA_RUN_UP_STEP_RIGHT, 0);
        }
    }

    if (Lara_TestSlide(item, coll)) {
        return;
    }

    if (coll->side_mid.floor >= 50) {
        item->pos.y += 50;
    } else {
        item->pos.y += coll->side_mid.floor;
    }
}

void Lara_Col_Stop(ITEM *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y;
    item->gravity = false;
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

    if (g_Config.gameplay.fix_descending_glitch && Lara_Fallen(item, coll)) {
        return;
    }

    if (coll->side_mid.floor > 100) {
        item->current_anim_state = LS_JUMP_FORWARD;
        item->goal_anim_state = LS_JUMP_FORWARD;
        Item_SwitchToAnim(item, LA_FALL_START, 0);
        item->gravity = true;
        item->fall_speed = 0;
        return;
    }

    if (Lara_TestSlide(item, coll)) {
        return;
    }

    Lara_ShiftCol(coll);
    item->pos.y += coll->side_mid.floor;
}

void Lara_Col_ForwardJump(ITEM *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = BAD_JUMP_CEILING;
    Lara_GetCollisionInfo(item, coll);

    Lara_DeflectEdgeJump(item, coll);

    if (item->fall_speed > 0 && coll->side_mid.floor <= 0) {
        if (Lara_LandedBad(item, coll)) {
            item->goal_anim_state = LS_DEATH;
        } else if (
            g_Lara.water_status != LWS_WADE && g_Input.forward
            && !g_Input.slow) {
            item->goal_anim_state = LS_RUN;
        } else {
            item->goal_anim_state = LS_STOP;
        }
        item->pos.y += coll->side_mid.floor;
        item->gravity = false;
        item->fall_speed = 0;
        item->speed = 0;

        if (!g_Config.gameplay.fix_wall_jump_glitch) {
            Lara_Animate(item);
        }
    }
}

void Lara_Col_FastBack(ITEM *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y - DEG_180;
    item->gravity = false;
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

    if (coll->side_mid.floor > 200) {
        item->current_anim_state = LS_FALL_BACK;
        item->goal_anim_state = LS_FALL_BACK;
        Item_SwitchToAnim(item, LA_FALL_BACK, 0);
        item->gravity = true;
        item->fall_speed = 0;
        return;
    }

    if (Lara_DeflectEdge(item, coll)) {
        Item_SwitchToAnim(item, LA_STAND_STILL, 0);
    }

    item->pos.y += coll->side_mid.floor;
}

void Lara_Col_Hang(ITEM *item, COLL_INFO *coll)
{
    Lara_HangTest(item, coll);
    if (item->goal_anim_state == LS_HANG && g_Input.forward) {
        if (coll->side_front.floor > -850 && coll->side_front.floor < -650
            && coll->side_front.floor - coll->side_front.ceiling >= 0
            && coll->side_left.floor - coll->side_left.ceiling >= 0
            && coll->side_right.floor - coll->side_right.ceiling >= 0
            && !coll->hit_static) {
            item->goal_anim_state = g_Input.slow ? LS_GYMNAST : LS_PULL_UP;
        }
    }
}

void Lara_Col_Back(ITEM *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y - DEG_180;
    item->gravity = false;
    item->fall_speed = 0;
    if (g_Lara.water_status == LWS_WADE) {
        coll->bad_pos = NO_BAD_POS;
    } else {
        coll->bad_pos = STEPUP_HEIGHT;
    }
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_walls = 1;
    coll->slopes_are_pits = 1;
    Lara_GetCollisionInfo(item, coll);

    if (Lara_HitCeiling(item, coll)) {
        return;
    }

    if (Lara_DeflectEdge(item, coll)) {
        Item_SwitchToAnim(item, LA_STAND_STILL, 0);
    }

    if (g_Config.gameplay.fix_descending_glitch && Lara_Fallen(item, coll)) {
        return;
    }

    if (coll->side_mid.floor > STEP_L / 2
        && coll->side_mid.floor < (STEP_L * 3) / 2) {
        if (Item_TestAnimEqual(item, LA_WALK_BACK)
            && Item_TestFrameRange(item, LF_BACK_R_START, LF_BACK_R_END)) {
            Item_SwitchToAnim(item, LA_WALK_DOWN_BACK_RIGHT, 0);
        } else {
            Item_SwitchToAnim(item, LA_WALK_DOWN_BACK_LEFT, 0);
        }
    }

    if (Lara_TestSlide(item, coll)) {
        return;
    }

    item->pos.y += coll->side_mid.floor;
}

void Lara_Col_SideStep(ITEM *item, COLL_INFO *coll)
{
    if (item->current_anim_state == LS_STEP_RIGHT) {
        g_Lara.move_angle = item->rot.y + DEG_90;
    } else {
        g_Lara.move_angle = item->rot.y - DEG_90;
    }

    item->gravity = false;
    item->fall_speed = 0;
    if (g_Lara.water_status == LWS_WADE) {
        coll->bad_pos = NO_BAD_POS;
    } else {
        coll->bad_pos = STEP_L / 2;
    }
    coll->bad_neg = -STEP_L / 2;
    coll->bad_ceiling = 0;
    coll->slopes_are_walls = 1;
    coll->slopes_are_pits = 1;
    Lara_GetCollisionInfo(item, coll);

    if (Lara_HitCeiling(item, coll)) {
        return;
    }

    if (Lara_DeflectEdge(item, coll)) {
        Item_SwitchToAnim(item, LA_STAND_STILL, 0);
    }

    if (g_Config.gameplay.fix_descending_glitch && Lara_Fallen(item, coll)) {
        return;
    }

    if (Lara_TestSlide(item, coll)) {
        return;
    }

    item->pos.y += coll->side_mid.floor;
}

void Lara_Col_UpJump(ITEM *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = BAD_JUMP_CEILING;
    coll->facing = g_Lara.move_angle;
    if (g_Config.gameplay.enable_lean_jumping && item->speed < 0) {
        coll->facing += DEG_180;
    }

    Collide_GetCollisionInfo(
        coll, item->pos.x, item->pos.y, item->pos.z, item->room_num, 870);

    if (Lara_TestHangJumpUp(item, coll)) {
        return;
    }

    Lara_SlideEdgeJump(item, coll);

    if (g_Config.gameplay.enable_lean_jumping) {
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

    if (item->fall_speed <= 0 || coll->side_mid.floor > 0) {
        return;
    }

    if (Lara_LandedBad(item, coll)) {
        item->goal_anim_state = LS_DEATH;
    } else {
        item->goal_anim_state = LS_STOP;
    }
    item->pos.y += coll->side_mid.floor;
    item->gravity = false;
    item->fall_speed = 0;
}

void Lara_Col_Roll(ITEM *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y;
    item->gravity = false;
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

    if (coll->side_mid.floor > 200) {
        item->current_anim_state = LS_JUMP_FORWARD;
        item->goal_anim_state = LS_JUMP_FORWARD;
        Item_SwitchToAnim(item, LA_FALL_START, 0);
        item->gravity = true;
        item->fall_speed = 0;
        return;
    }

    Lara_ShiftCol(coll);
    item->pos.y += coll->side_mid.floor;
}

void Lara_Col_SurfSwim(ITEM *item, COLL_INFO *coll)
{
    coll->bad_neg = -STEPUP_HEIGHT;
    g_Lara.move_angle = item->rot.y;
    Lara_SurfaceCollision(item, coll);
    if (g_Config.gameplay.enable_wading) {
        Lara_TestWaterClimbOut(item, coll);
    }
}

void Lara_Col_SurfTread(ITEM *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y;
    Lara_SurfaceCollision(item, coll);
}

void Lara_Col_SurfBack(ITEM *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y - DEG_180;
    Lara_SurfaceCollision(item, coll);
}

void Lara_Col_SurfLeft(ITEM *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y - DEG_90;
    Lara_SurfaceCollision(item, coll);
}

void Lara_Col_SurfRight(ITEM *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y + DEG_90;
    Lara_SurfaceCollision(item, coll);
}

void Lara_Col_Wade(ITEM *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y;
    coll->slopes_are_walls = 1;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;

    Lara_GetCollisionInfo(item, coll);
    if (Lara_HitCeiling(item, coll) || Lara_TestVault(item, coll)) {
        return;
    }

    if (Lara_DeflectEdge(item, coll)) {
        item->rot.z = 0;
        if (coll->side_front.type == HT_WALL
            && coll->side_front.floor < -STEP_L * 5 / 2
            && coll->old_anim_state == LS_WADE
            && Item_TestAnimEqual(item, LA_WADE)) {
            item->current_anim_state = LS_SPLAT;
            if (Item_TestFrameRange(item, LF_WADE_L_START, LF_WADE_L_END)) {
                Item_SwitchToAnim(item, LA_WALL_SMASH_LEFT, 0);
                return;
            }
            if (Item_TestFrameRange(item, LF_WADE_R_START, LF_WADE_R_END)) {
                Item_SwitchToAnim(item, LA_WALL_SMASH_RIGHT, 0);
                return;
            }
        }
        M_CollideStop(item, coll);
    }

    if (Lara_Fallen(item, coll)) {
        return;
    }

    if (coll->side_mid.floor >= -STEPUP_HEIGHT
        && coll->side_mid.floor < -STEP_L / 2) {
        if (Item_TestFrameRange(
                item, LF_WADE_STEP_L_START, LF_WADE_STEP_L_END)) {
            Item_SwitchToAnim(item, LA_RUN_UP_STEP_LEFT, 0);
        } else {
            Item_SwitchToAnim(item, LA_RUN_UP_STEP_RIGHT, 0);
        }
    }

    if (Lara_TestSlide(item, coll)) {
        return;
    }

    item->pos.y += MIN(coll->side_mid.floor, 50);
}
