#include <trx/game/lara/interact.h>

#include <trx/config.h>
#include <trx/game/input.h>
#include <trx/game/lara.h>
#include <trx/version.h>

bool Lara_Interact_CanBegin(const LARA_INTERACT_MODE mode)
{
    const ITEM *const lara_item = Lara_GetItem();
    const LARA_TRX_ANIMATION anim = LA_U(Item_GetRelativeAnim(lara_item));
    const LARA_TRX_STATE state = LS_U(lara_item->current_anim_state);
    if (g_TRVersion < 3) {
        if (anim == LA_SPRINT_SLIDE_STAND_RIGHT
            || anim == LA_SPRINT_SLIDE_STAND_LEFT) {
            return false;
        }
        if (state == LS_STOP) {
            return true;
        }
    }

    if (state == LS_STOP && anim == LA_STAND_IDLE) {
        return true;
    }

    if (mode != LARA_INTERACT_PICKUP) {
        return false;
    }

    return (state == LS_CROUCH_IDLE && anim == LA_CROUCH_IDLE)
        || (state == LS_CRAWL_IDLE && anim == LA_CRAWL_IDLE
            && g_Config.gameplay.enable_responsive_crawl);
}

bool Lara_Interact_CanControl(
    const LARA_INTERACT_MODE mode, const int16_t item_num)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->interact_target.is_moving
        && lara->interact_target.item_num == item_num) {
        return true;
    }

    if (!g_Input.action || lara->gun_status != LGS_ARMLESS) {
        return false;
    }

    const ITEM *const lara_item = Lara_GetItem();
    if (lara_item->gravity) {
        return false;
    }

    if (lara_item->current_anim_state != LS(LS_STOP)) {
        return false;
    }

    switch (mode) {
    case LARA_INTERACT_PICKUP:
        return !lara->interact_target.is_moving;
    case LARA_INTERACT_SWITCH:
        const ITEM *const item = Item_Get(item_num);
        return item->status == IS_INACTIVE;
    default:
        return true;
    }
}
