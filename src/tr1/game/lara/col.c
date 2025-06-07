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

#define LF_BACK_R_START 26
#define LF_BACK_R_END 55

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
