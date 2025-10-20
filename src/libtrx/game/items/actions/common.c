#include "game/items/actions.h"
#include "game/rooms.h"

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
