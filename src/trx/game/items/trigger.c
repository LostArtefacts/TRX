#include <trx/game/const.h>
#include <trx/game/game.h>
#include <trx/game/items/manager.h>
#include <trx/game/items/utils.h>
#include <trx/game/lua/events.h>
#include <trx/game/objects.h>
#include <trx/version.h>

#include <math.h>

// Fire the on_trigger event: a trigger of any kind was aimed at the item, with
// its fundamentals. Held here, next to Item_Trigger, so the primitive stays
// clear of the event stack.
static void M_FireTriggerEvent(
    const int16_t item_num, const ITEM_TRIGGER *const trigger)
{
    if (Game_IsSettingUpItems()) {
        return;
    }
    const LUA_EVENT_ARG args[] = {
        { .type = LUA_EVENT_ARG_INT32, .value = { .i32 = item_num } },
        { .type = LUA_EVENT_ARG_INT32, .value = { .i32 = trigger->kind } },
        // The mask the way a level editor counts it, 1 to 31.
        { .type = LUA_EVENT_ARG_INT32, .value = { .i32 = trigger->mask } },
        { .type = LUA_EVENT_ARG_NUMBER, .value = { .number = trigger->timer } },
        { .type = LUA_EVENT_ARG_BOOL, .value = { .b = trigger->one_shot } },
    };
    LUA_FireEventEx(LUA_EVENT_TRIGGER, args, 5);
}

// Whether a spent one-shot trigger of this kind must be ignored. TR3 latches a
// spent switch and a spent antitrigger in their own bits, apart from the
// general one-shot; a heavy switch uses the general one-shot like everything
// else.
static bool M_IsOneShotSpent(
    const ITEM_TRIGGER *const trigger, const ITEM *const item)
{
    if (item->object_id == O_PROPELLER_1 || item->object_id == O_PROPELLER_2) {
        return false;
    }

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

        if (Item_IsTriggerActiveRO(item)) {
            if (trigger->one_shot) {
                item->trigger.spent = true;
            }
            Item_Activate(item_num, false);
        }
    }

    // Fired last, so a handler sees the item as the trigger left it, and
    // nothing it does to the item is then overwritten from under it.
    M_FireTriggerEvent(item_num, trigger);
}
