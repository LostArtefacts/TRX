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
            return (item->flags & IF_ONE_SHOT_SWITCH) != 0;
        case ITEM_TRIGGER_ANTI:
            return (item->flags & IF_ONE_SHOT_ANTITRIGGER) != 0;
        default:
            break;
        }
    }
    return (item->flags & IF_ONE_SHOT) != 0;
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
    if (obj->trigger_func != nullptr && !obj->trigger_func(item, trigger)) {
        return;
    }

    switch (trigger->kind) {
    case ITEM_TRIGGER_SWITCH:
    case ITEM_TRIGGER_HEAVY_SWITCH:
        item->flags ^= trigger->mask;
        if (trigger->one_shot && g_TRVersion == 3) {
            item->flags |= IF_ONE_SHOT_SWITCH;
        }
        break;

    case ITEM_TRIGGER_ANTI:
        // TODO investigate unifying as ~(trigger->mask | IF_REVERSE)
        if (g_TRVersion >= 3) {
            item->flags &= ~(IF_CODE_BITS | IF_REVERSE);
        } else {
            item->flags &= ~trigger->mask;
        }
        if (trigger->one_shot) {
            item->flags |=
                g_TRVersion == 3 ? IF_ONE_SHOT_ANTITRIGGER : IF_ONE_SHOT;
        }
        break;

    default:
        item->flags |= trigger->mask;
        break;
    }

    if ((item->flags & IF_CODE_BITS) != IF_CODE_BITS) {
        return;
    }

    if (trigger->one_shot) {
        item->flags |= IF_ONE_SHOT;
    }
    Item_Activate(item_num, false);
}
