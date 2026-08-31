#include <trx/game/lara/interact.h>

#include <trx/config.h>
#include <trx/game/input.h>
#include <trx/game/lara.h>
#include <trx/game/objects/families.h>

bool Lara_Interact_HasActiveTarget(const int16_t item_num)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    return lara->interact_target.is_moving
        && lara->interact_target.item_num == item_num;
}

bool Lara_Interact_HasActiveType(const LARA_INTERACT_MODE mode)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->interact_target.item_num == NO_ITEM
        || !lara->interact_target.is_moving) {
        return false;
    }

    const ITEM *const item = Item_Get(lara->interact_target.item_num);
    switch (mode) {
    case LARA_INTERACT_PICKUP:
        return item->object_id == O_FLARE_ITEM
            || ObjectFamily_Has(item->object_id, OBJ_FAMILY_PICKUP);
    case LARA_INTERACT_RECEPTACLE:
        return ObjectFamily_Has(item->object_id, OBJ_FAMILY_RECEPTACLE);
    case LARA_INTERACT_SWITCH:
    case LARA_INTERACT_FLOOR_SWITCH:
        return ObjectFamily_Has(item->object_id, OBJ_FAMILY_SWITCH);
    case LARA_INTERACT_DOOR:
        return ObjectFamily_Has(item->object_id, OBJ_FAMILY_DOOR);
    default:
        return false;
    }
}

bool Lara_Interact_CanBegin(const LARA_INTERACT_MODE mode)
{
    const ITEM *const lara_item = Lara_GetItem();
    const LARA_ANIMATION_ID anim = LA_U(Item_GetRelativeAnim(lara_item));
    const LARA_STATE_ID state = LS_U(lara_item->current_anim_state);
    if (g_Config.gameplay.enable_snap_interactions) {
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
    if (Lara_Interact_HasActiveTarget(item_num)) {
        return true;
    }

    const LARA_INFO *const lara = Lara_GetLaraInfo();
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

    if (!g_Config.gameplay.enable_snap_interactions
        && Item_GetRelativeAnim(lara_item) != LA(LA_STAND_IDLE)) {
        return false;
    }

    switch (mode) {
    case LARA_INTERACT_PICKUP:
        return !lara->interact_target.is_moving;
    case LARA_INTERACT_SWITCH:
        return Item_IsInactive(Item_Get(item_num));
    case LARA_INTERACT_DOOR:
        return !Item_IsInPlay(Item_Get(item_num));
    default:
        return true;
    }
}

void Lara_Interact_FinishControl(const LARA_INTERACT_MODE mode)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->head_rot.y = 0;
    lara->head_rot.x = 0;
    lara->torso_rot.y = 0;
    lara->torso_rot.x = 0;
    lara->gun_status = LGS_HANDS_BUSY;

    lara->interact_target.is_moving = false;
    if (mode == LARA_INTERACT_SWITCH || mode == LARA_INTERACT_FLOOR_SWITCH) {
        lara->interact_target.item_num = NO_ITEM;
    }
}
