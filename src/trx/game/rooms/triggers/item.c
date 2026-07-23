#include <trx/game/items.h>
#include <trx/game/rooms.h>

// The item-activation semantics only distinguish a handful of behaviors, so the
// floordata trigger type collapses onto an ITEM_TRIGGER_KIND here, at the
// rooms->items boundary. Everything past this point works in item terms.
static ITEM_TRIGGER_KIND M_MapKind(const TRIGGER_TYPE type)
{
    switch (type) {
    case TT_SWITCH:
        return ITEM_TRIGGER_SWITCH;
    case TT_HEAVY_SWITCH:
        return ITEM_TRIGGER_HEAVY_SWITCH;
    case TT_HEAVY:
        return ITEM_TRIGGER_HEAVY;
    case TT_ANTIPAD:
    case TT_ANTITRIGGER:
    case TT_HEAVY_ANTITRIGGER:
        return ITEM_TRIGGER_ANTI;
    default:
        return ITEM_TRIGGER_NORMAL;
    }
}

static void M_Handle(
    const TRIGGER *const trigger, const TRIGGER_CMD *const cmd,
    TRIGGER_STATUS *const status)
{
    const int16_t item_num = (int16_t)(intptr_t)cmd->parameter;
    const ITEM_TRIGGER_KIND kind = M_MapKind(trigger->type);

    // A heavy switch toggles by the mask the heavy object carries, not the one
    // authored on the trigger; folding it in here keeps the item code from ever
    // having to know about heavy objects.
    const ITEM_TRIGGER item_trigger = {
        .kind = kind,
        .mask = (int16_t)(kind == ITEM_TRIGGER_HEAVY_SWITCH ? status->heavy_mask
                                                            : trigger->mask),
        .timer = trigger->timer,
        .one_shot = trigger->one_shot,
    };
    Item_Trigger(item_num, &item_trigger);
}

REGISTER_TRIGGER_HANDLER(TO_ITEM, M_Handle)
