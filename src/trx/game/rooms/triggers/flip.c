#include <trx/game/rooms.h>

static void M_HandleFlipMap(
    const TRIGGER *const trigger, const TRIGGER_CMD *const cmd,
    TRIGGER_STATUS *const status)
{
    const int16_t flip_slot = (int16_t)(intptr_t)cmd->parameter;
    int32_t slot_flags = Room_GetFlipSlotFlags(flip_slot);
    status->flip_available = true;

    if ((slot_flags & IF_ONE_SHOT) != 0) {
        return;
    }

    if (trigger->type == TT_SWITCH) {
        slot_flags ^= trigger->mask;
    } else {
        slot_flags |= trigger->mask;
    }

    if ((slot_flags & IF_CODE_BITS) == IF_CODE_BITS) {
        if (trigger->one_shot) {
            slot_flags |= IF_ONE_SHOT;
        }

        if (!status->flip_status) {
            status->flip_map = true;
        }
    } else if (status->flip_status) {
        status->flip_map = true;
    }

    Room_SetFlipSlotFlags(flip_slot, slot_flags);
}

static void M_HandleFlipOn(
    const TRIGGER *const trigger, const TRIGGER_CMD *const cmd,
    TRIGGER_STATUS *const status)
{
    const int16_t flip_slot = (int16_t)(intptr_t)cmd->parameter;
    const int32_t slot_flags = Room_GetFlipSlotFlags(flip_slot);
    status->flip_available = true;

    if ((slot_flags & IF_CODE_BITS) == IF_CODE_BITS && !status->flip_status) {
        status->flip_map = true;
    }
}

static void M_HandleFlipOff(
    const TRIGGER *const trigger, const TRIGGER_CMD *const cmd,
    TRIGGER_STATUS *const status)
{
    const int16_t flip_slot = (int16_t)(intptr_t)cmd->parameter;
    const int32_t slot_flags = Room_GetFlipSlotFlags(flip_slot);
    status->flip_available = true;

    if ((slot_flags & IF_CODE_BITS) == IF_CODE_BITS && status->flip_status) {
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
