#include "game/input.h"
#include "game/lara.h"
#include "game/lara/util.h"

#define M_LF_HANG 21
#define M_LF_CLIMB_L_SHIFT_START 28
#define M_LF_CLIMB_L_SHIFT_END 29
#define M_LF_CLIMB_R_SHIFT 57

// TODO: move all dependent climb functions from misc.c here
static bool M_GetClimbStatus(void);
static bool M_TestLadderRelease(ITEM *item);

static void M_Hang(ITEM *item, COLL_INFO *coll);
static void M_Shimmy(ITEM *item, COLL_INFO *coll);
static void M_StanceLadder(ITEM *item, COLL_INFO *coll);
static void M_SideLadder(ITEM *item, COLL_INFO *coll);
static void M_UpLadder(ITEM *item, COLL_INFO *coll);
static void M_DownLadder(ITEM *item, COLL_INFO *coll);

static bool M_GetClimbStatus(void)
{
#if TR_VERSION == 1
    return false;
#else
    return Lara_GetLaraInfo()->climb_status;
#endif
}

static bool M_TestLadderRelease(ITEM *const item)
{
    item->gravity = false;
    item->fall_speed = 0;

    if (g_Input.action && item->hit_points > 0) {
        return false;
    }

    item->goal_anim_state = LS_JUMP_FORWARD;
    item->current_anim_state = LS_JUMP_FORWARD;
    Item_SwitchToAnim(item, LA_FALL_START, 0);
    item->gravity = true;
    item->speed = 2;
    item->fall_speed = 1;
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->gun_status = LGS_ARMLESS;
    return true;
}

static void M_Hang(ITEM *const item, COLL_INFO *const coll)
{
    Lara_HangTest(item, coll);
    if (item->goal_anim_state != LS_HANG) {
        return;
    }

    const bool climb_status = M_GetClimbStatus();
    if (g_Input.forward) {
        if (coll->side_front.floor <= -850 || coll->side_front.floor >= -650
            || coll->side_front.floor - coll->side_front.ceiling < 0
            || coll->side_left.floor - coll->side_left.ceiling < 0
            || coll->side_right.floor - coll->side_right.ceiling < 0
            || coll->hit_static) {
#if TR_VERSION >= 2
            if (climb_status && Item_TestAnimEqual(item, LA_REACH_TO_HANG)
                && Item_TestFrameEqual(item, M_LF_HANG)
                && coll->side_mid.ceiling <= -256) {
                item->goal_anim_state = LS_HANG;
                item->current_anim_state = LS_HANG;
                Item_SwitchToAnim(item, LA_LADDER_UP_HANGING, 0);
            }
#endif
        } else {
            item->goal_anim_state = g_Input.slow ? LS_GYMNAST : LS_PULL_UP;
        }
    } else if (
        g_Input.back && climb_status
        && Item_TestAnimEqual(item, LA_REACH_TO_HANG)
        && Item_TestFrameEqual(item, M_LF_HANG)) {
#if TR_VERSION >= 2
        item->goal_anim_state = LS_HANG;
        item->current_anim_state = LS_HANG;
        Item_SwitchToAnim(item, LA_LADDER_DOWN_HANGING, 0);
#endif
    }
}

static void M_Shimmy(ITEM *const item, COLL_INFO *const coll)
{
    const int32_t angle =
        item->current_anim_state == LS_SHIMMY_LEFT ? -DEG_90 : DEG_90;
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->move_angle = item->rot.y + angle;
    Lara_HangTest(item, coll);
    lara->move_angle = item->rot.y + angle;
}

static void M_StanceLadder(ITEM *const item, COLL_INFO *const coll)
{
#if TR_VERSION >= 2
    if (M_TestLadderRelease(item)
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
#endif
}

static void M_SideLadder(ITEM *const item, COLL_INFO *const coll)
{
#if TR_VERSION >= 2
    if (M_TestLadderRelease(item)) {
        return;
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    int32_t right;
    if (item->current_anim_state == LS_CLIMB_LEFT) {
        lara->move_angle = item->rot.y - DEG_90;
        right = -(coll->radius + LARA_CLIMB_WIDTH_LEFT);
    } else {
        lara->move_angle = item->rot.y + DEG_90;
        right = coll->radius + LARA_CLIMB_WIDTH_RIGHT;
    }

    int32_t shift;
    int32_t result = Lara_TestClimbPos(
        item, coll->radius, right, -LARA_CLIMB_HEIGHT, LARA_CLIMB_HEIGHT,
        &shift);
    Lara_DoClimbLeftRight(item, coll, result, shift);
#endif
}

static void M_UpLadder(ITEM *const item, COLL_INFO *const coll)
{
#if TR_VERSION >= 2
    if (M_TestLadderRelease(item) || !Item_TestAnimEqual(item, LA_LADDER_UP)) {
        return;
    }

    int32_t yshift;
    if (Item_TestFrameEqual(item, 0)) {
        yshift = 0;
    } else if (Item_TestFrameRange(
                   item, M_LF_CLIMB_L_SHIFT_START, M_LF_CLIMB_L_SHIFT_END)) {
        yshift = -STEP_L;
    } else if (Item_TestFrameEqual(item, M_LF_CLIMB_R_SHIFT)) {
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
#endif
}

static void M_DownLadder(ITEM *const item, COLL_INFO *const coll)
{
#if TR_VERSION >= 2
    if (M_TestLadderRelease(item)
        || !Item_TestAnimEqual(item, LA_LADDER_DOWN)) {
        return;
    }

    int32_t yshift;
    if (Item_TestFrameEqual(item, 0)) {
        yshift = 0;
    } else if (Item_TestFrameRange(
                   item, M_LF_CLIMB_L_SHIFT_START, M_LF_CLIMB_L_SHIFT_END)) {
        yshift = STEP_L;
    } else if (Item_TestFrameEqual(item, M_LF_CLIMB_R_SHIFT)) {
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
#endif
}

// clang-format off
REGISTER_LARA_COL(LS_HANG,         M_Hang)
REGISTER_LARA_COL(LS_SHIMMY_LEFT,  M_Shimmy)
REGISTER_LARA_COL(LS_SHIMMY_RIGHT, M_Shimmy)
#if TR_VERSION >= 2
REGISTER_LARA_COL(LS_CLIMB_STANCE, M_StanceLadder)
REGISTER_LARA_COL(LS_CLIMB_LEFT,   M_SideLadder)
REGISTER_LARA_COL(LS_CLIMB_RIGHT,  M_SideLadder)
REGISTER_LARA_COL(LS_CLIMBING,     M_UpLadder)
REGISTER_LARA_COL(LS_CLIMB_DOWN,   M_DownLadder)
#endif
// clang-format on
