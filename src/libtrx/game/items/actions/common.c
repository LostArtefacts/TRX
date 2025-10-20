#include "game/items/actions.h"
#include "game/rooms.h"

static void (*m_Routines[ITEM_ACTION_NUMBER_OF])(ITEM *item) = {};

void Item_ActionRegister(
    const ITEM_TRX_ACTION action, void (*const action_func)(ITEM *item))
{
    m_Routines[action] = action_func;
}

void Item_ActionRun(const ITEM_TRX_ACTION action_id, ITEM *const item)
{
    if (action_id >= 0 && action_id < ITEM_ACTION_NUMBER_OF
        && m_Routines[action_id] != nullptr) {
        m_Routines[action_id](item);
    }
}

void Item_ActionRunDirect(const ITEM_ACTION action_id, ITEM *const item)
{
    const ITEM_TRX_ACTION trx_id = Item_ActionFromGameID(action_id);
    Item_ActionRun(trx_id, item);
}

void Item_ActionRunActive(void)
{
    const int32_t flip_effect = Room_GetFlipEffect();
    if (flip_effect != -1) {
        Item_ActionRunDirect(flip_effect, nullptr);
    }
}
