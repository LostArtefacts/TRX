#include "game/items/actions.h"

void Item_ActionRunDirect(const ITEM_ACTION action_id, ITEM *const item)
{
    const ITEM_TRX_ACTION trx_id = Item_ActionFromGameID(action_id);
    Item_ActionRun(trx_id, item);
}
