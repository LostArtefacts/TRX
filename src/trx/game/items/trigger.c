#include <trx/game/const.h>
#include <trx/game/items/manager.h>
#include <trx/game/items/utils.h>
#include <trx/game/objects.h>
#include <trx/version.h>

#include <math.h>

// Whether a spent one-shot trigger of this kind must be ignored. TR3 latches a
// spent switch and a spent antitrigger in their own bits, apart from the
// general one-shot; a heavy switch uses the general one-shot like everything
// else.
static bool M_IsOneShotSpent(
    const ITEM_TRIGGER *const trigger, const ITEM *const item)
{
    if (g_TRVersion == 3) {
        switch (trigger->kind) {
        case ITEM_TRIGGER_SWITCH:
            return item->trigger.switch_spent;
        case ITEM_TRIGGER_ANTI:
            return item->trigger.anti_spent;
        default:
            break;
        }
    }
    return item->trigger.spent;
}

void Item_Trigger(const int16_t item_num, const ITEM_TRIGGER *const trigger)
{
    ITEM *const item = Item_Get(item_num);
    if (M_IsOneShotSpent(trigger, item)) {
        return;
    }

    // A timer of one second is the sentinel for a single frame, not thirty.
    item->timer = trigger->timer == 1.0f
        ? 1
        : (int16_t)lroundf(trigger->timer * LOGIC_FPS);

    const OBJECT *const obj = Object_Get(item->object_id);
    if (obj->trigger_func == nullptr || obj->trigger_func(item, trigger)) {
        switch (trigger->kind) {
        case ITEM_TRIGGER_SWITCH:
        case ITEM_TRIGGER_HEAVY_SWITCH:
            item->trigger.mask ^= trigger->mask;
            if (trigger->one_shot && g_TRVersion == 3) {
                item->trigger.switch_spent = true;
            }
            break;

        case ITEM_TRIGGER_ANTI:
            if (g_TRVersion >= 3) {
                item->trigger.mask = 0;
                item->trigger.reversed = false;
            } else {
                item->trigger.mask &= ~trigger->mask;
            }
            if (trigger->one_shot) {
                if (g_TRVersion == 3) {
                    item->trigger.anti_spent = true;
                } else {
                    item->trigger.spent = true;
                }
            }
            break;

        default:
            item->trigger.mask |= trigger->mask;
            break;
        }

        if (item->trigger.mask == TRIGGER_MASK_ALL) {
            if (trigger->one_shot) {
                item->trigger.spent = true;
            }
            Item_Activate(item_num, false);
        }
    }

    // Fired last, so a handler sees the item as the trigger left it, and
    // nothing it does to the item is then overwritten from under it.
    Item_NotifyTriggered(item_num, trigger);
}
