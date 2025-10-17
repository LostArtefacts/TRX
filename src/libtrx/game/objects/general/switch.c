#include "game/objects/general/switch.h"

#include "game/const.h"
#include "game/items.h"

bool Switch_Trigger(const int16_t item_num, const int16_t timer)
{
    ITEM *const item = Item_Get(item_num);

    if (item->object_id == O_SWITCH_TYPE_AIRLOCK) {
        if (item->status == IS_DEACTIVATED) {
            Item_RemoveActive(item_num);
            item->status = IS_INACTIVE;
            return false;
        } else if (
            (item->flags & IF_ONE_SHOT) != 0
            || item->current_anim_state == SWITCH_STATE_OFF) {
            return false;
        }

        item->flags |= IF_ONE_SHOT;
        return true;
    }

    if (item->status != IS_DEACTIVATED) {
        return false;
    }

    if (item->current_anim_state == SWITCH_STATE_OFF && timer > 0) {
        item->timer = timer;
        if (timer != 1) {
            item->timer *= LOGIC_FPS;
        }
        item->status = IS_ACTIVE;
    } else {
        Item_RemoveActive(item_num);
        item->status = IS_INACTIVE;
    }
    return true;
}
