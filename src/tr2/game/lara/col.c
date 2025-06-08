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

#define LF_CLIMB_L_SHIFT_START 28
#define LF_CLIMB_L_SHIFT_END 29
#define LF_CLIMB_R_SHIFT 57

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
