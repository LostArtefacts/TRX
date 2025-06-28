#include "config.h"
#include "game/input.h"
#include "game/lara.h"
#include "game/lara/util.h"

static void M_Compress(ITEM *item, COLL_INFO *coll);
static void M_UpJump(ITEM *item, COLL_INFO *coll);

static void M_Compress(ITEM *const item, COLL_INFO *const coll)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->water_status != LWS_WADE) {
        if (g_Input.forward
            && Lara_FloorFront(item, item->rot.y, STEP_L) >= -STEPUP_HEIGHT) {
            item->goal_anim_state = LS_JUMP_FORWARD;
            lara->move_angle = item->rot.y;
        } else if (
            g_Input.left
            && Lara_FloorFront(item, item->rot.y - DEG_90, STEP_L)
                >= -STEPUP_HEIGHT) {
            item->goal_anim_state = LS_JUMP_LEFT;
            lara->move_angle = item->rot.y - DEG_90;
        } else if (
            g_Input.right
            && Lara_FloorFront(item, item->rot.y + DEG_90, STEP_L)
                >= -STEPUP_HEIGHT) {
            item->goal_anim_state = LS_JUMP_RIGHT;
            lara->move_angle = item->rot.y + DEG_90;
        } else if (
            g_Input.back
            && Lara_FloorFront(item, item->rot.y + DEG_180, STEP_L)
                >= -STEPUP_HEIGHT) {
            item->goal_anim_state = LS_JUMP_BACK;
            lara->move_angle = item->rot.y + DEG_180;
        }
    }

    if (item->fall_speed > LARA_FAST_FALL_SPEED) {
        item->goal_anim_state = LS_FAST_FALL;
    }
}

static void M_UpJump(ITEM *item, COLL_INFO *coll)
{
    const int16_t fast_speed = g_Config.gameplay.enable_swing_cancel
        ? LARA_SWING_FAST_FALL_SPEED
        : LARA_FAST_FALL_SPEED;
    if (item->fall_speed > fast_speed) {
        item->goal_anim_state = LS_FAST_FALL;
    }
}

// clang-format off
REGISTER_LARA_STATE(LS_COMPRESS,     M_Compress)
REGISTER_LARA_STATE(LS_JUMP_UP,      M_UpJump)
// clang-format on
