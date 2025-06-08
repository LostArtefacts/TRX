#include "game/input.h"
#include "game/lara/control.h"
#include "game/lara/misc.h"
#include "game/sound.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/collision.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/math.h>
#include <libtrx/utils.h>

#define LF_BACK_R_START 26
#define LF_BACK_R_END 55

#define LF_HANG 21
#define LF_CLIMB_L_SHIFT_START 28
#define LF_CLIMB_L_SHIFT_END 29
#define LF_CLIMB_R_SHIFT 57

static void M_CollideStop(ITEM *item, const COLL_INFO *coll);
static bool M_Fallen(ITEM *item, const COLL_INFO *coll);

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

static bool M_Fallen(ITEM *const item, const COLL_INFO *const coll)
{
    if (coll->side_mid.floor <= STEPUP_HEIGHT
        || g_Lara.water_status == LWS_WADE) {
        return false;
    }
    item->current_anim_state = LS_JUMP_FORWARD;
    item->goal_anim_state = LS_JUMP_FORWARD;
    Item_SwitchToAnim(item, LA_FALL_START, 0);
    item->gravity = true;
    item->fall_speed = 0;
    return true;
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
            item->goal_anim_state = LS_PULL_UP;
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

void Lara_Col_Back(ITEM *item, COLL_INFO *coll)
{
    item->gravity = false;
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
        M_CollideStop(item, coll);
    }

    if (M_Fallen(item, coll)) {
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
    coll->slopes_are_pits = 1;
    coll->slopes_are_walls = 1;
    coll->bad_neg = -STEP_L / 2;
    coll->bad_ceiling = 0;

    Lara_GetCollisionInfo(item, coll);
    if (Lara_HitCeiling(item, coll)) {
        return;
    }

    if (Lara_DeflectEdge(item, coll)) {
        M_CollideStop(item, coll);
    }

    if (!M_Fallen(item, coll) && !Lara_TestSlide(item, coll)) {
        item->pos.y += coll->side_mid.floor;
    }
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
        if (item->goal_anim_state == LS_PULL_UP) {
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
            item->goal_anim_state = LS_PULL_UP;
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
            item->goal_anim_state = LS_PULL_UP;
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
