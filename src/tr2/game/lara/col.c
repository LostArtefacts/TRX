#include "game/lara/col.h"

#include "game/collide.h"
#include "game/input.h"
#include "game/items.h"
#include "game/lara/control.h"
#include "game/lara/misc.h"
#include "game/room.h"
#include "game/sound.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/lara/const.h>
#include <libtrx/game/math.h>
#include <libtrx/utils.h>

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

#define LF_HANG 21
#define LF_CLIMB_L_SHIFT_START 28
#define LF_CLIMB_L_SHIFT_END 29
#define LF_CLIMB_R_SHIFT 57

void Lara_CollideStop(ITEM *const item, const COLL_INFO *const coll)
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

bool Lara_Fallen(ITEM *const item, const COLL_INFO *const coll)
{
    if (coll->side_mid.floor <= STEPUP_HEIGHT
        || g_Lara.water_status == LWS_WADE) {
        return false;
    }
    item->current_anim_state = LS_JUMP_FORWARD;
    item->goal_anim_state = LS_JUMP_FORWARD;
    Item_SwitchToAnim(item, LA_FALL_START, 0);
    item->gravity = 1;
    item->fall_speed = 0;
    return true;
}

bool Lara_TestWaterClimbOut(ITEM *const item, const COLL_INFO *const coll)
{
    if (coll->coll_type != COLL_FRONT || !g_Input.action
        || coll->side_front.type == HT_BIG_SLOPE) {
        return false;
    }

    const int32_t coll_hdif =
        ABS(coll->side_left.floor - coll->side_right.floor);
    const int32_t min_coll_hdif = 60;
    if (coll_hdif >= min_coll_hdif) {
        return false;
    }

    if (g_Lara.gun_status != LGS_ARMLESS
        && (g_Lara.gun_status != LGS_READY || g_Lara.gun_type != LGT_FLARE)) {
        return false;
    }

    if (coll->side_front.ceiling > 0) {
        return false;
    }
    if (coll->side_mid.ceiling > -STEPUP_HEIGHT) {
        return false;
    }

    const int32_t lara_hdif = coll->side_front.floor + LARA_HEIGHT_SURF;
    if (lara_hdif <= -STEP_L * 2
        || lara_hdif > LARA_HEIGHT_SURF - STEPUP_HEIGHT) {
        return false;
    }

    const DIRECTION dir = Math_GetDirectionCone(item->rot.y, 35 * DEG_1);
    if (dir == DIR_UNKNOWN) {
        return false;
    }

    item->pos.y += lara_hdif - 5;
    Item_UpdateRoom(item, -LARA_HEIGHT / 2);

    switch (dir) {
    case DIR_NORTH:
        item->pos.z = (item->pos.z & -WALL_L) + WALL_L + LARA_RADIUS;
        break;
    case DIR_WEST:
        item->pos.x = (item->pos.x & -WALL_L) + WALL_L + LARA_RADIUS;
        break;
    case DIR_SOUTH:
        item->pos.z = (item->pos.z & -WALL_L) - LARA_RADIUS;
        break;
    case DIR_EAST:
        item->pos.x = (item->pos.x & -WALL_L) - LARA_RADIUS;
        break;
    case DIR_UNKNOWN:
        return false;
    }

    if (lara_hdif < -STEP_L / 2) {
        Item_SwitchToAnim(item, LA_ONWATER_TO_STAND_HIGH, 0);
    } else if (lara_hdif < STEP_L / 2) {
        Item_SwitchToAnim(item, LA_ONWATER_TO_STAND_MEDIUM, 0);
    } else {
        Item_SwitchToAnim(item, LA_ONWATER_TO_WADE_LOW, 0);
    }

    item->current_anim_state = LS_WATER_OUT;
    item->goal_anim_state = LS_STOP;
    item->rot.y = Math_DirectionToAngle(dir);
    item->rot.x = 0;
    item->rot.z = 0;
    item->gravity = 0;
    item->speed = 0;
    item->fall_speed = 0;
    g_Lara.gun_status = LGS_HANDS_BUSY;
    g_Lara.water_status = LWS_ABOVE_WATER;
    return true;
}

bool Lara_TestWaterStepOut(ITEM *const item, const COLL_INFO *const coll)
{
    if (coll->coll_type == COLL_FRONT || coll->side_mid.type == HT_BIG_SLOPE
        || coll->side_mid.floor >= 0) {
        return false;
    }

    if (coll->side_mid.floor < -STEP_L / 2) {
        item->current_anim_state = LS_WATER_OUT;
        item->goal_anim_state = LS_STOP;
        Item_SwitchToAnim(item, LA_ONWATER_TO_WADE, 0);
    } else if (item->goal_anim_state == LS_SURF_LEFT) {
        item->goal_anim_state = LS_STEP_LEFT;
    } else if (item->goal_anim_state == LS_SURF_RIGHT) {
        item->goal_anim_state = LS_STEP_RIGHT;
    } else {
        item->current_anim_state = LS_WADE;
        item->goal_anim_state = LS_WADE;
        Item_SwitchToAnim(item, LA_WADE, 0);
    }

    item->pos.y += coll->side_front.floor + LARA_HEIGHT_SURF - 5;
    Item_UpdateRoom(item, -LARA_HEIGHT / 2);
    item->gravity = 0;
    item->rot.x = 0;
    item->rot.z = 0;
    item->speed = 0;
    item->fall_speed = 0;
    g_Lara.water_status = LWS_WADE;
    return true;
}

void Lara_SurfaceCollision(ITEM *const item, COLL_INFO *const coll)
{
    coll->facing = g_Lara.move_angle;
    Collide_GetCollisionInfo(
        coll, item->pos.x, item->pos.y + LARA_HEIGHT_SURF, item->pos.z,
        item->room_num, LARA_HEIGHT_SURF + 100);

    Item_ShiftCol(item, coll);

    if (coll->coll_type == COLL_LEFT) {
        item->rot.y += 5 * DEG_1;
    } else if (coll->coll_type == COLL_RIGHT) {
        item->rot.y -= 5 * DEG_1;
    } else if (
        coll->coll_type != COLL_NONE
        || (coll->side_mid.floor < 0 && coll->side_mid.type == HT_BIG_SLOPE)) {
        item->fall_speed = 0;
        item->pos.x = coll->old.x;
        item->pos.y = coll->old.y;
        item->pos.z = coll->old.z;
    }

    const int32_t water_height = Room_GetWaterHeight(
        item->pos.x, item->pos.y, item->pos.z, item->room_num);
    if (water_height - item->pos.y <= -100) {
        item->current_anim_state = LS_DIVE;
        item->goal_anim_state = LS_SWIM;
        Item_SwitchToAnim(item, LA_ONWATER_DIVE, 0);
        item->rot.x = -45 * DEG_1;
        item->fall_speed = 80;
        g_Lara.water_status = LWS_UNDERWATER;
        return;
    }

    Lara_TestWaterStepOut(item, coll);
}

void Lara_Col_Empty(ITEM *item, COLL_INFO *coll)
{
}

void Lara_Col_Walk(ITEM *item, COLL_INFO *coll)
{
    item->gravity = 0;
    item->fall_speed = 0;
    g_Lara.move_angle = item->rot.y;
    coll->slopes_are_pits = 1;
    coll->slopes_are_walls = 1;
    coll->lava_is_pit = 1;
    coll->bad_pos = STEPUP_HEIGHT;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    Lara_GetCollisionInfo(item, coll);

    if (Lara_HitCeiling(item, coll) || Lara_TestVault(item, coll)) {
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
            Lara_CollideStop(item, coll);
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
        item->gravity = 0;
        item->fall_speed = 0;
    }

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
        if (Item_TestAnimEqual(item, LA_RUN)
            && Lara_TestWall(item, STEP_L, 0, -STEP_L * 5 / 2)) {
            item->current_anim_state = LS_SPLAT;
            if (Item_TestFrameRange(item, LF_RUN_L_START, LF_RUN_L_END)) {
                Item_SwitchToAnim(item, LA_WALL_SMASH_LEFT, 0);
                return;
            }
            if (Item_TestFrameRange(item, LF_RUN_R_START, LF_RUN_R_END)) {
                Item_SwitchToAnim(item, LA_WALL_SMASH_RIGHT, 0);
                return;
            }
        }
        Lara_CollideStop(item, coll);
    }

    if (Lara_Fallen(item, coll)) {
        return;
    }

    if (coll->side_mid.floor >= -STEPUP_HEIGHT
        && coll->side_mid.floor < -STEP_L / 2) {
        if (g_Config.gameplay.fix_step_glitch
            && (coll->side_front.floor < -STEPUP_HEIGHT
                || coll->side_front.floor >= -STEP_L / 2)) {
            coll->side_mid.floor = 0;
        } else {
            if (Item_TestFrameRange(
                    item, LF_RUN_L_HEEL_GROUND, LF_RUN_R_FOOT_GROUND)) {
                Item_SwitchToAnim(item, LA_RUN_UP_STEP_LEFT, 0);
            } else {
                Item_SwitchToAnim(item, LA_RUN_UP_STEP_RIGHT, 0);
            }
        }
    }

    if (Lara_TestSlide(item, coll)) {
        return;
    }

    item->pos.y += MIN(coll->side_mid.floor, 50);
}

void Lara_Col_Stop(ITEM *item, COLL_INFO *coll)
{
    item->gravity = 0;
    item->fall_speed = 0;
    g_Lara.move_angle = item->rot.y;
    coll->slopes_are_pits = 1;
    coll->slopes_are_walls = 1;
    coll->bad_pos = STEPUP_HEIGHT;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    Lara_GetCollisionInfo(item, coll);

    if (Lara_HitCeiling(item, coll) || Lara_Fallen(item, coll)
        || Lara_TestSlide(item, coll)) {
        return;
    }

    Item_ShiftCol(item, coll);
    item->pos.y += coll->side_mid.floor;
}

void Lara_Col_ForwardJump(ITEM *item, COLL_INFO *coll)
{
    if (item->speed < 0) {
        g_Lara.move_angle = item->rot.y + DEG_180;
    } else {
        g_Lara.move_angle = item->rot.y;
    }
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = BAD_JUMP_CEILING;

    Lara_GetCollisionInfo(item, coll);
    Lara_DeflectEdgeJump(item, coll);
    if (item->speed < 0) {
        g_Lara.move_angle = item->rot.y;
    }

    if (coll->side_mid.floor > 0 || item->fall_speed <= 0) {
        return;
    }

    if (Lara_LandedBad(item, coll)) {
        item->goal_anim_state = LS_DEATH;
    } else if (
        g_Lara.water_status != LWS_WADE && g_Input.forward && !g_Input.slow) {
        item->goal_anim_state = LS_RUN;
    } else {
        item->goal_anim_state = LS_STOP;
    }

    item->gravity = 0;
    item->fall_speed = 0;
    item->pos.y += coll->side_mid.floor;
    item->speed = 0;
    Lara_Animate(item);
}

void Lara_Col_FastBack(ITEM *item, COLL_INFO *coll)
{
    item->gravity = 0;
    item->fall_speed = 0;
    g_Lara.move_angle = item->rot.y + DEG_180;
    coll->slopes_are_pits = 1;
    coll->slopes_are_walls = 1;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;

    Lara_GetCollisionInfo(item, coll);
    if (Lara_HitCeiling(item, coll)) {
        return;
    }

    if (coll->side_mid.floor <= 200) {
        if (Lara_DeflectEdge(item, coll)) {
            Lara_CollideStop(item, coll);
        }
        item->pos.y += coll->side_mid.floor;
    } else {
        Item_SwitchToAnim(item, LA_FALL_BACK, 0);
        item->current_anim_state = LS_FALL_BACK;
        item->goal_anim_state = LS_FALL_BACK;
        item->gravity = 1;
        item->fall_speed = 0;
    }
}

void Lara_Col_TurnRight(ITEM *item, COLL_INFO *coll)
{
    item->gravity = 0;
    item->fall_speed = 0;
    g_Lara.move_angle = item->rot.y;
    coll->slopes_are_pits = 1;
    coll->slopes_are_walls = 1;
    coll->bad_pos = STEPUP_HEIGHT;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;

    Lara_GetCollisionInfo(item, coll);

    if (coll->side_mid.floor <= 100) {
        if (!Lara_TestSlide(item, coll)) {
            item->pos.y += coll->side_mid.floor;
        }
    } else {
        Item_SwitchToAnim(item, LA_FALL_START, 0);
        item->current_anim_state = LS_JUMP_FORWARD;
        item->goal_anim_state = LS_JUMP_FORWARD;
        item->gravity = 1;
        item->fall_speed = 0;
    }
}

void Lara_Col_TurnLeft(ITEM *item, COLL_INFO *coll)
{
    Lara_Col_TurnRight(item, coll);
}

void Lara_Col_Death(ITEM *item, COLL_INFO *coll)
{
    Sound_StopEffect(SFX_LARA_FALL);
    g_Lara.move_angle = item->rot.y;
    coll->bad_pos = STEPUP_HEIGHT;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->radius = 400;

    Lara_GetCollisionInfo(item, coll);
    Item_ShiftCol(item, coll);

    item->pos.y += coll->side_mid.floor;
    item->hit_points = -1;
    g_Lara.air = -1;
}

void Lara_Col_FastFall(ITEM *item, COLL_INFO *coll)
{
    item->gravity = 1;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = BAD_JUMP_CEILING;

    Lara_GetCollisionInfo(item, coll);
    Lara_SlideEdgeJump(item, coll);
    if (coll->side_mid.floor > 0) {
        return;
    }

    if (Lara_LandedBad(item, coll)) {
        item->goal_anim_state = LS_DEATH;
    } else {
        item->goal_anim_state = LS_STOP;
        item->current_anim_state = LS_STOP;
        Item_SwitchToAnim(item, LA_FREEFALL_LAND, 0);
    }

    Sound_StopEffect(SFX_LARA_FALL);
    item->gravity = 0;
    item->fall_speed = 0;
    item->pos.y += coll->side_mid.floor;
}

void Lara_Col_Hang(ITEM *item, COLL_INFO *coll)
{
    Lara_HangTest(item, coll);
    if (item->goal_anim_state != LS_HANG) {
        return;
    }

    if (g_Input.forward) {
        if (coll->side_front.floor <= -850 || coll->side_front.floor >= -650
            || coll->side_front.floor - coll->side_front.ceiling < 0
            || coll->side_left.floor - coll->side_left.ceiling < 0
            || coll->side_right.floor - coll->side_right.ceiling < 0
            || coll->hit_static) {
            if (g_Lara.climb_status
                && Item_TestAnimEqual(item, LA_REACH_TO_HANG)
                && Item_TestFrameEqual(item, LF_HANG)
                && coll->side_mid.ceiling <= -256) {
                item->goal_anim_state = LS_HANG;
                item->current_anim_state = LS_HANG;
                Item_SwitchToAnim(item, LA_LADDER_UP_HANGING, 0);
            }
        } else if (g_Input.slow) {
            item->goal_anim_state = LS_GYMNAST;
        } else {
            item->goal_anim_state = LS_NULL;
        }
    } else if (
        g_Input.back && g_Lara.climb_status
        && Item_TestAnimEqual(item, LA_REACH_TO_HANG)
        && Item_TestFrameEqual(item, LF_HANG)) {
        item->goal_anim_state = LS_HANG;
        item->current_anim_state = LS_HANG;
        Item_SwitchToAnim(item, LA_LADDER_DOWN_HANGING, 0);
    }
}

void Lara_Col_Reach(ITEM *item, COLL_INFO *coll)
{
    item->gravity = 1;
    g_Lara.move_angle = item->rot.y;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = 0;
    coll->bad_ceiling = BAD_JUMP_CEILING;

    Lara_GetCollisionInfo(item, coll);
    if (Lara_TestHangJump(item, coll)) {
        return;
    }

    Lara_SlideEdgeJump(item, coll);
    if (item->fall_speed <= 0 || coll->side_mid.floor > 0) {
        return;
    }

    if (Lara_LandedBad(item, coll)) {
        item->goal_anim_state = LS_DEATH;
    } else {
        item->goal_anim_state = LS_STOP;
    }
    item->gravity = 0;
    item->fall_speed = 0;
    item->pos.y += coll->side_mid.floor;
}

void Lara_Col_Splat(ITEM *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y;
    coll->slopes_are_pits = 1;
    coll->slopes_are_walls = 1;
    coll->bad_pos = STEPUP_HEIGHT;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;

    Lara_GetCollisionInfo(item, coll);
    Item_ShiftCol(item, coll);

    if (coll->side_mid.floor > -STEP_L && coll->side_mid.floor < STEP_L) {
        item->pos.y += coll->side_mid.floor;
    }
}

void Lara_Col_Land(ITEM *item, COLL_INFO *coll)
{
    Lara_Col_Stop(item, coll);
}

void Lara_Col_Compress(ITEM *item, COLL_INFO *coll)
{
    item->gravity = 0;
    item->fall_speed = 0;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = NO_BAD_NEG;
    coll->bad_ceiling = 0;

    Lara_GetCollisionInfo(item, coll);

    if (coll->side_mid.ceiling > -100) {
        Item_SwitchToAnim(item, LA_STAND_STILL, 0);
        item->goal_anim_state = LS_STOP;
        item->current_anim_state = LS_STOP;
        item->gravity = 0;
        item->speed = 0;
        item->fall_speed = 0;
        item->pos.x = coll->old.x;
        item->pos.y = coll->old.y;
        item->pos.z = coll->old.z;
    }

    if (coll->side_mid.floor > -STEP_L && coll->side_mid.floor < STEP_L) {
        item->pos.y += coll->side_mid.floor;
    }
}

void Lara_Col_Back(ITEM *item, COLL_INFO *coll)
{
    item->gravity = 0;
    item->fall_speed = 0;
    g_Lara.move_angle = item->rot.y + DEG_180;
    if (g_Lara.water_status == LWS_WADE) {
        coll->bad_pos = NO_BAD_POS;
    } else {
        coll->bad_pos = STEPUP_HEIGHT;
    }
    coll->slopes_are_pits = 1;
    coll->slopes_are_walls = 1;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;

    Lara_GetCollisionInfo(item, coll);
    if (Lara_HitCeiling(item, coll)) {
        return;
    }

    if (Lara_DeflectEdge(item, coll)) {
        Lara_CollideStop(item, coll);
    }

    if (Lara_Fallen(item, coll)) {
        return;
    }

    if (coll->side_mid.floor > STEP_L / 2
        && coll->side_mid.floor < STEPUP_HEIGHT) {
        if (Item_TestFrameRange(item, LF_BACK_R_START, LF_BACK_R_END)) {
            Item_SwitchToAnim(item, LA_WALK_DOWN_BACK_RIGHT, 0);
        } else {
            Item_SwitchToAnim(item, LA_WALK_DOWN_BACK_LEFT, 0);
        }
    }

    if (!Lara_TestSlide(item, coll)) {
        item->pos.y += coll->side_mid.floor;
    }
}

void Lara_Col_StepRight(ITEM *item, COLL_INFO *coll)
{
    if (item->current_anim_state == LS_STEP_RIGHT) {
        g_Lara.move_angle = item->rot.y + DEG_90;
    } else {
        g_Lara.move_angle = item->rot.y - DEG_90;
    }

    item->gravity = 0;
    item->fall_speed = 0;
    if (g_Lara.water_status == LWS_WADE) {
        coll->bad_pos = NO_BAD_POS;
    } else {
        coll->bad_pos = STEP_L / 2;
    }
    coll->slopes_are_pits = 1;
    coll->slopes_are_walls = 1;
    coll->bad_neg = -STEP_L / 2;
    coll->bad_ceiling = 0;

    Lara_GetCollisionInfo(item, coll);
    if (Lara_HitCeiling(item, coll)) {
        return;
    }

    if (Lara_DeflectEdge(item, coll)) {
        Lara_CollideStop(item, coll);
    }

    if (!Lara_Fallen(item, coll) && !Lara_TestSlide(item, coll)) {
        item->pos.y += coll->side_mid.floor;
    }
}

void Lara_Col_StepLeft(ITEM *item, COLL_INFO *coll)
{
    Lara_Col_StepRight(item, coll);
}

void Lara_Col_Slide(ITEM *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y;
    Lara_SlideSlope(item, coll);
}

void Lara_Col_BackJump(ITEM *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y + DEG_180;
    Lara_Col_Jumper(item, coll);
}

void Lara_Col_RightJump(ITEM *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y + DEG_90;
    Lara_Col_Jumper(item, coll);
}

void Lara_Col_LeftJump(ITEM *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y - DEG_90;
    Lara_Col_Jumper(item, coll);
}

void Lara_Col_UpJump(ITEM *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = BAD_JUMP_CEILING;
    if (item->speed < 0) {
        coll->facing = g_Lara.move_angle + DEG_180;
    } else {
        coll->facing = g_Lara.move_angle;
    }

    Collide_GetCollisionInfo(
        coll, item->pos.x, item->pos.y, item->pos.z, item->room_num, 870);
    if (Lara_TestHangJumpUp(item, coll)) {
        return;
    }

    Lara_SlideEdgeJump(item, coll);
    if (coll->coll_type != COLL_NONE) {
        item->speed = item->speed > 0 ? 2 : -2;
    } else if (item->fall_speed < -70) {
        if (g_Input.forward && item->speed < 5) {
            item->speed++;
        } else if (g_Input.back && item->speed > -5) {
            item->speed -= 2;
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
    item->gravity = 0;
    item->fall_speed = 0;
    item->pos.y += coll->side_mid.floor;
}

void Lara_Col_Fallback(ITEM *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y + DEG_180;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = BAD_JUMP_CEILING;

    Lara_GetCollisionInfo(item, coll);
    Lara_DeflectEdgeJump(item, coll);

    if (coll->side_mid.floor > 0 || item->fall_speed <= 0) {
        return;
    }

    if (Lara_LandedBad(item, coll)) {
        item->goal_anim_state = LS_DEATH;
    } else {
        item->goal_anim_state = LS_STOP;
    }

    item->gravity = 0;
    item->fall_speed = 0;
    item->pos.y += coll->side_mid.floor;
}

void Lara_Col_HangLeft(ITEM *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y - DEG_90;
    Lara_HangTest(item, coll);
    g_Lara.move_angle = item->rot.y - DEG_90;
}

void Lara_Col_HangRight(ITEM *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y + DEG_90;
    Lara_HangTest(item, coll);
    g_Lara.move_angle = item->rot.y + DEG_90;
}

void Lara_Col_SlideBack(ITEM *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y + DEG_180;
    Lara_SlideSlope(item, coll);
}

void Lara_Col_Null(ITEM *item, COLL_INFO *coll)
{
    Lara_Col_Default(item, coll);
}

void Lara_Col_Roll(ITEM *item, COLL_INFO *coll)
{
    item->gravity = 0;
    item->fall_speed = 0;
    g_Lara.move_angle = item->rot.y;
    coll->slopes_are_walls = 1;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;

    Lara_GetCollisionInfo(item, coll);
    if (Lara_HitCeiling(item, coll) || Lara_TestSlide(item, coll)
        || Lara_Fallen(item, coll)) {
        return;
    }

    Item_ShiftCol(item, coll);
    item->pos.y += coll->side_mid.floor;
}

void Lara_Col_Roll2(ITEM *item, COLL_INFO *coll)
{
    item->gravity = 0;
    item->fall_speed = 0;
    g_Lara.move_angle = item->rot.y + DEG_180;
    coll->slopes_are_walls = 1;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;

    Lara_GetCollisionInfo(item, coll);
    if (Lara_HitCeiling(item, coll) || Lara_TestSlide(item, coll)) {
        return;
    }

    if (coll->side_mid.floor > 200) {
        Item_SwitchToAnim(item, LA_FALL_BACK, 0);
        item->current_anim_state = LS_FALL_BACK;
        item->goal_anim_state = LS_FALL_BACK;
        item->gravity = 1;
        item->fall_speed = 0;
    } else {
        Item_ShiftCol(item, coll);
        item->pos.y += coll->side_mid.floor;
    }
}

void Lara_Col_SwanDive(ITEM *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = BAD_JUMP_CEILING;

    Lara_GetCollisionInfo(item, coll);
    Lara_DeflectEdgeJump(item, coll);
    if (coll->side_mid.floor > 0 || item->fall_speed <= 0) {
        return;
    }

    item->goal_anim_state = LS_STOP;
    item->gravity = 0;
    item->fall_speed = 0;
    item->pos.y += coll->side_mid.floor;
}

void Lara_Col_FastDive(ITEM *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = BAD_JUMP_CEILING;

    Lara_GetCollisionInfo(item, coll);
    Lara_DeflectEdgeJump(item, coll);

    if (coll->side_mid.floor > 0 || item->fall_speed <= 0) {
        return;
    }

    if (item->fall_speed > 133) {
        item->goal_anim_state = LS_DEATH;
    } else {
        item->goal_anim_state = LS_STOP;
    }
    item->gravity = 0;
    item->fall_speed = 0;
    item->pos.y += coll->side_mid.floor;
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
        if (coll->side_front.type != COLL_NONE
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
        Lara_CollideStop(item, coll);
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

void Lara_Col_Default(ITEM *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y;
    coll->slopes_are_walls = 1;
    coll->slopes_are_pits = 1;
    coll->bad_pos = STEPUP_HEIGHT;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    Lara_GetCollisionInfo(item, coll);
}

void Lara_Col_Jumper(ITEM *item, COLL_INFO *coll)
{
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = BAD_JUMP_CEILING;

    Lara_GetCollisionInfo(item, coll);
    Lara_DeflectEdgeJump(item, coll);
    if (item->fall_speed <= 0 || coll->side_mid.floor > 0) {
        return;
    }

    if (Lara_LandedBad(item, coll)) {
        item->goal_anim_state = LS_DEATH;
    } else {
        item->goal_anim_state = LS_STOP;
    }
    item->gravity = 0;
    item->fall_speed = 0;
    item->pos.y += coll->side_mid.floor;
}

void Lara_Col_ClimbLeft(ITEM *item, COLL_INFO *coll)
{
    if (Lara_CheckForLetGo(item, coll)) {
        return;
    }
    g_Lara.move_angle = item->rot.y - DEG_90;

    int32_t shift;
    int32_t result = Lara_TestClimbPos(
        item, coll->radius, -(coll->radius + LARA_CLIMB_WIDTH_LEFT),
        -LARA_CLIMB_HEIGHT, LARA_CLIMB_HEIGHT, &shift);

    Lara_DoClimbLeftRight(item, coll, result, shift);
}

void Lara_Col_ClimbRight(ITEM *item, COLL_INFO *coll)
{
    if (Lara_CheckForLetGo(item, coll)) {
        return;
    }
    g_Lara.move_angle = item->rot.y + DEG_90;

    int32_t shift;
    int32_t result = Lara_TestClimbPos(
        item, coll->radius, coll->radius + LARA_CLIMB_WIDTH_RIGHT,
        -LARA_CLIMB_HEIGHT, LARA_CLIMB_HEIGHT, &shift);
    Lara_DoClimbLeftRight(item, coll, result, shift);
}

void Lara_Col_ClimbStance(ITEM *item, COLL_INFO *coll)
{
    if (Lara_CheckForLetGo(item, coll)
        || !Item_TestAnimEqual(item, LA_LADDER_IDLE)) {
        return;
    }

    if (g_Input.forward) {
        if (item->goal_anim_state == LS_NULL) {
            return;
        }

        item->goal_anim_state = LS_CLIMB_STANCE;

        int32_t shift_r = 0;
        int32_t ledge_r = 0;
        int32_t result_r = Lara_TestClimbUpPos(
            item, coll->radius, coll->radius + LARA_CLIMB_WIDTH_RIGHT, &shift_r,
            &ledge_r);

        int32_t shift_l = 0;
        int32_t ledge_l = 0;
        int32_t result_l = Lara_TestClimbUpPos(
            item, coll->radius, -(coll->radius + LARA_CLIMB_WIDTH_LEFT),
            &shift_l, &ledge_l);

        if (!result_r || !result_l) {
            return;
        }

        if (result_r < 0 || result_l < 0) {
            if (ABS(ledge_l - ledge_r) > 120) {
                return;
            }
            item->goal_anim_state = LS_NULL;
            item->pos.y += (ledge_l + ledge_r) / 2 - STEP_L;
            return;
        }

        int32_t shift = shift_l;
        if (shift_r) {
            if (shift_l) {
                if ((shift_r < 0) != (shift_l < 0)) {
                    return;
                }
                if (shift_r > 0 && shift_r > shift_l) {
                    shift = shift_r;
                } else if (shift_r < 0 && shift_r < shift_l) {
                    shift = shift_r;
                }
            } else {
                shift = shift_r;
            }
        }

        item->goal_anim_state = LS_CLIMBING;
        item->pos.y += shift;
    } else if (g_Input.back) {
        if (item->goal_anim_state == LS_HANG) {
            return;
        }

        item->goal_anim_state = LS_CLIMB_STANCE;
        item->pos.y += STEP_L;

        int32_t shift_r = 0;
        int32_t result_r = Lara_TestClimbPos(
            item, coll->radius, coll->radius + LARA_CLIMB_WIDTH_RIGHT,
            -LARA_CLIMB_HEIGHT, LARA_CLIMB_HEIGHT, &shift_r);

        int32_t shift_l = 0;
        int32_t result_l = Lara_TestClimbPos(
            item, coll->radius, -(coll->radius + LARA_CLIMB_WIDTH_LEFT),
            -LARA_CLIMB_HEIGHT, LARA_CLIMB_HEIGHT, &shift_l);

        item->pos.y -= STEP_L;
        if (!result_r || !result_l) {
            return;
        }

        int32_t shift = shift_l;
        if (shift_r && shift_l) {
            if ((shift_r < 0) != (shift_l < 0)) {
                return;
            }
            if (shift_r < 0 && shift_r < shift_l) {
                shift = shift_r;
            } else if (shift_r > 0 && shift_r > shift_l) {
                shift = shift_r;
            }
        }

        if (result_r == 1 && result_l == 1) {
            item->goal_anim_state = LS_CLIMB_DOWN;
            item->pos.y += shift;
        } else {
            item->goal_anim_state = LS_HANG;
        }
    }
}

void Lara_Col_Climbing(ITEM *item, COLL_INFO *coll)
{
    if (Lara_CheckForLetGo(item, coll)
        || !Item_TestAnimEqual(item, LA_LADDER_UP)) {
        return;
    }

    int32_t yshift;
    if (Item_TestFrameEqual(item, 0)) {
        yshift = 0;
    } else if (Item_TestFrameRange(
                   item, LF_CLIMB_L_SHIFT_START, LF_CLIMB_L_SHIFT_END)) {
        yshift = -STEP_L;
    } else if (Item_TestFrameEqual(item, LF_CLIMB_R_SHIFT)) {
        yshift = -STEP_L * 2;
    } else {
        return;
    }

    item->pos.y += yshift - STEP_L;

    int32_t shift_r = 0;
    int32_t ledge_r = 0;
    int32_t result_r = Lara_TestClimbUpPos(
        item, coll->radius, coll->radius + LARA_CLIMB_WIDTH_RIGHT, &shift_r,
        &ledge_r);

    int32_t shift_l = 0;
    int32_t ledge_l = 0;
    int32_t result_l = Lara_TestClimbUpPos(
        item, coll->radius, -(coll->radius + LARA_CLIMB_WIDTH_LEFT), &shift_l,
        &ledge_l);

    item->pos.y += STEP_L;

    if (!result_r || !result_l || !g_Input.forward) {
        item->goal_anim_state = LS_CLIMB_STANCE;
        if (yshift) {
            Lara_Animate(item);
        }
        return;
    }

    if (result_r < 0 || result_l < 0) {
        item->goal_anim_state = LS_CLIMB_STANCE;
        Lara_Animate(item);
        if (ABS(ledge_l - ledge_r) <= 120) {
            item->goal_anim_state = LS_NULL;
            item->pos.y += (ledge_r + ledge_l) / 2 - STEP_L;
        }
        return;
    }

    item->goal_anim_state = LS_CLIMBING;
    item->pos.y -= yshift;
}

void Lara_Col_ClimbDown(ITEM *item, COLL_INFO *coll)
{
    if (Lara_CheckForLetGo(item, coll)
        || !Item_TestAnimEqual(item, LA_LADDER_DOWN)) {
        return;
    }

    int32_t yshift;
    if (Item_TestFrameEqual(item, 0)) {
        yshift = 0;
    } else if (Item_TestFrameRange(
                   item, LF_CLIMB_L_SHIFT_START, LF_CLIMB_L_SHIFT_END)) {
        yshift = STEP_L;
    } else if (Item_TestFrameEqual(item, LF_CLIMB_R_SHIFT)) {
        yshift = STEP_L * 2;
    } else {
        return;
    }

    item->pos.y += yshift + STEP_L;

    int32_t shift_r = 0;
    int32_t result_r = Lara_TestClimbPos(
        item, coll->radius, coll->radius + LARA_CLIMB_WIDTH_RIGHT,
        -LARA_CLIMB_HEIGHT, LARA_CLIMB_HEIGHT, &shift_r);

    int32_t shift_l = 0;
    int32_t result_l = Lara_TestClimbPos(
        item, coll->radius, -(coll->radius + LARA_CLIMB_WIDTH_LEFT),
        -LARA_CLIMB_HEIGHT, LARA_CLIMB_HEIGHT, &shift_l);

    item->pos.y -= STEP_L;

    if (!result_r || !result_l || !g_Input.back) {
        item->goal_anim_state = LS_CLIMB_STANCE;
        if (yshift) {
            Lara_Animate(item);
        }
        return;
    }

#if 0
    int32_t shift = shift_l;
#endif
    if (shift_r && shift_l) {
        if ((shift_r < 0) != (shift_l < 0)) {
            item->goal_anim_state = LS_CLIMB_STANCE;
            Lara_Animate(item);
            return;
        }
#if 0
        if (shift_r < 0 && shift_r < shift_l) {
            shift = shift_r;
        } else if (shift_r > 0 && shift_r > shift_l) {
            shift = shift_r;
        }
#endif
    }

    if (result_r == -1 || result_l == -1) {
        Item_SwitchToAnim(item, LA_LADDER_IDLE, 0);
        item->current_anim_state = LS_CLIMB_STANCE;
        item->goal_anim_state = LS_HANG;
        Lara_Animate(item);
        return;
    }

    item->goal_anim_state = LS_CLIMB_DOWN;
    item->pos.y -= yshift;
}

void Lara_Col_SurfSwim(ITEM *item, COLL_INFO *coll)
{
    coll->bad_neg = -STEPUP_HEIGHT;
    g_Lara.move_angle = item->rot.y;
    Lara_SurfaceCollision(item, coll);
    Lara_TestWaterClimbOut(item, coll);
}

void Lara_Col_SurfBack(ITEM *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y + DEG_180;
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

void Lara_Col_SurfTread(ITEM *item, COLL_INFO *coll)
{
    g_Lara.move_angle = item->rot.y;
    Lara_SurfaceCollision(item, coll);
}

void Lara_Col_Swim(ITEM *item, COLL_INFO *coll)
{
    Lara_SwimCollision(item, coll);
}

void Lara_Col_UWDeath(ITEM *item, COLL_INFO *coll)
{
    item->hit_points = -1;
    g_Lara.air = -1;
    g_Lara.gun_status = LGS_HANDS_BUSY;
    int32_t wh = Room_GetWaterHeight(
        item->pos.x, item->pos.y, item->pos.z, item->room_num);
    if (wh != NO_HEIGHT && wh < item->pos.y - 100) {
        item->pos.y -= 5;
    }
    Lara_SwimCollision(item, coll);
}
