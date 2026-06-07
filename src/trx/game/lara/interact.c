#include <trx/game/lara/interact.h>

#include <trx/config.h>
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
