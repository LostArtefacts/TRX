#include <trx/game/items.h>
#include <trx/game/objects.h>
#include <trx/game/objects/general/shoal.h>
#include <trx/game/pathing.h>
#include <trx/game/rooms.h>
#include <trx/version.h>

static bool M_IsOneShot(const TRIGGER *const trigger, const ITEM *const item)
{
    if (g_TRVersion == 3) {
        switch (trigger->type) {
        case TT_SWITCH:
            return (item->flags & IF_ONE_SHOT_SWITCH) != 0;
        case TT_ANTIPAD:
        case TT_ANTITRIGGER:
        case TT_HEAVY_ANTITRIGGER:
            return (item->flags & IF_ONE_SHOT_ANTITRIGGER) != 0;
        default:
            break;
        }
    }

    return (item->flags & IF_ONE_SHOT) != 0;
}

static void M_Handle(
    const TRIGGER *const trigger, const TRIGGER_CMD *const cmd,
    TRIGGER_STATUS *const status)
{
    const int16_t item_num = (int16_t)(intptr_t)cmd->parameter;
    ITEM *const item = Item_Get(item_num);

    if (M_IsOneShot(trigger, item)) {
        return;
    }

    item->timer = trigger->timer;
    if (item->timer != 1) {
        item->timer *= LOGIC_FPS;
    }

    const OBJECT *const obj = Object_Get(item->object_id);
    if (obj->trigger_func != nullptr) {
        const bool use_default_handling = obj->trigger_func(item, trigger);
        if (!use_default_handling) {
            return;
        }
    }

    if (trigger->type == TT_SWITCH || trigger->type == TT_HEAVY_SWITCH) {
        if (trigger->type == TT_HEAVY_SWITCH) {
            item->flags ^= status->heavy_mask;
        } else {
            item->flags ^= trigger->mask;
        }
        if (trigger->one_shot && g_TRVersion == 3) {
            item->flags |= IF_ONE_SHOT_SWITCH;
        }
    } else if (
        trigger->type == TT_ANTIPAD || trigger->type == TT_ANTITRIGGER
        || trigger->type == TT_HEAVY_ANTITRIGGER) {
        // TODO investigate unifying as ~(trigger->mask | IF_REVERSE)
        if (g_TRVersion >= 3) {
            item->flags &= ~(IF_CODE_BITS | IF_REVERSE);
        } else {
            item->flags &= ~trigger->mask;
        }
        if (trigger->one_shot) {
            if (g_TRVersion == 3) {
                item->flags |= IF_ONE_SHOT_ANTITRIGGER;
            } else {
                item->flags |= IF_ONE_SHOT;
            }
        }
    } else {
        item->flags |= trigger->mask;
    }

    if ((item->flags & IF_CODE_BITS) != IF_CODE_BITS) {
        return;
    }

    if (trigger->one_shot) {
        item->flags |= IF_ONE_SHOT;
    }

    if (item->active) {
        return;
    }

    if (obj->activate_func != nullptr) {
        obj->activate_func(item);
    } else if (obj->intelligent) {
        if (item->status == IS_INACTIVE) {
            item->touch_bits = 0;
            item->status = IS_ACTIVE;
            Item_AddActive(item_num);
            LOT_EnableBaddieAI(item_num, true);
        } else if (item->status == IS_INVISIBLE) {
            item->touch_bits = 0;
            if (LOT_EnableBaddieAI(item_num, false)) {
                item->status = IS_ACTIVE;
            } else {
                item->status = IS_INVISIBLE;
            }
            Item_AddActive(item_num);
        }
    } else {
        item->touch_bits = 0;
        item->status = IS_ACTIVE;
        Item_AddActive(item_num);
    }
}

REGISTER_TRIGGER_HANDLER(TO_ITEM, M_Handle)
