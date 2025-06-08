#include "game/lara/col.h"

#include "debug.h"
#include "game/input.h"
#include "game/lara.h"

static void (*m_CollisionRoutines[LS_NUMBER_OF])(
    ITEM *item, COLL_INFO *coll) = {};

void Lara_Col_Register(
    const LARA_STATE state,
    void (*const handle_func)(ITEM *item, COLL_INFO *coll))
{
    ASSERT(state >= 0 && state < LS_NUMBER_OF);
    m_CollisionRoutines[state] = handle_func;
}

void Lara_Col_Update(ITEM *const item, COLL_INFO *const coll)
{
    if (m_CollisionRoutines[item->current_anim_state] != nullptr) {
        m_CollisionRoutines[item->current_anim_state](item, coll);
    }
}

bool Lara_Col_Fallen(ITEM *const item, const COLL_INFO *const coll)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (coll->side_mid.floor <= STEPUP_HEIGHT
        || lara->water_status == LWS_WADE) {
        return false;
    }
    item->current_anim_state = LS_JUMP_FORWARD;
    item->goal_anim_state = LS_JUMP_FORWARD;
    Item_SwitchToAnim(item, LA_FALL_START, 0);
    item->gravity = true;
    item->fall_speed = 0;
    return true;
}

void Lara_Col_Stop(ITEM *const item, const COLL_INFO *const coll)
{
#if TR_VERSION == 1
    // TODO: this routine gives smoother recovery after splatting against a wall
    // - offer it fully in TR1 as its only scope is currently for wading.
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->water_status != LWS_WADE) {
        Item_SwitchToAnim(item, LA_STAND_STILL, 0);
        return;
    }
#endif

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
