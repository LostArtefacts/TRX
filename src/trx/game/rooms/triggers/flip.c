#include <trx/game/rooms.h>

static void M_HandleFlipMap(
    const TRIGGER *const trigger, const TRIGGER_CMD *const cmd,
    TRIGGER_STATUS *const status)
{
    const int16_t flip_slot = (int16_t)(intptr_t)cmd->parameter;
    status->flip_available = true;

    if (Room_GetFlipSlot(flip_slot)->is_one_shot) {
        return;
    }

    const FLIP_TRIGGER flip_trigger = {
        .from_switch = trigger->type == TT_SWITCH,
        .mask = trigger->mask,
        .one_shot = trigger->one_shot,
    };
    if (Room_TriggerFlipSlot(flip_slot, &flip_trigger)) {
        if (!status->flip_status) {
            status->flip_map = true;
        }
    } else if (status->flip_status) {
        status->flip_map = true;
    }
}

static void M_HandleFlipOn(
    const TRIGGER *const trigger, const TRIGGER_CMD *const cmd,
    TRIGGER_STATUS *const status)
{
    const int16_t flip_slot = (int16_t)(intptr_t)cmd->parameter;
    const FLIP_SLOT *const slot = Room_GetFlipSlot(flip_slot);
    status->flip_available = true;

    if ((slot->mask & FSF_CODE_BITS) == FSF_CODE_BITS && !status->flip_status) {
        status->flip_map = true;
    }
}

static void M_HandleFlipOff(
    const TRIGGER *const trigger, const TRIGGER_CMD *const cmd,
    TRIGGER_STATUS *const status)
{
    const int16_t flip_slot = (int16_t)(intptr_t)cmd->parameter;
    const FLIP_SLOT *const slot = Room_GetFlipSlot(flip_slot);
    status->flip_available = true;

    if ((slot->mask & FSF_CODE_BITS) == FSF_CODE_BITS && status->flip_status) {
        status->flip_map = true;
    }
}

static void M_HandleFlipEffect(
    const TRIGGER *const trigger, const TRIGGER_CMD *const cmd,
    TRIGGER_STATUS *const status)
{
    status->new_effect = (int16_t)(intptr_t)cmd->parameter;
}

REGISTER_TRIGGER_HANDLER(TO_FLIP_MAP, M_HandleFlipMap)
REGISTER_TRIGGER_HANDLER(TO_FLIP_ON, M_HandleFlipOn)
REGISTER_TRIGGER_HANDLER(TO_FLIP_OFF, M_HandleFlipOff)
REGISTER_TRIGGER_HANDLER(TO_FLIP_EFFECT, M_HandleFlipEffect)
