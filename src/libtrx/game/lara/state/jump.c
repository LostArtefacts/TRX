#include "game/input.h"
#include "game/lara.h"
#include "game/lara/util.h"

static void M_Compress(ITEM *item, COLL_INFO *coll);

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

// clang-format off
REGISTER_LARA_STATE(LS_COMPRESS,     M_Compress)
// clang-format on
