#include <trx/game/catalog/manager.h>
#include <trx/game/items.h>

ITEM_ACTION ItemAction_ToGameID(const ITEM_TRX_ACTION action)
{
    return Catalog_IDToSlot(CATALOG_ITEM_ACTIONS, action, ITEM_ACTION_INVALID);
}

ITEM_TRX_ACTION ItemAction_FromGameID(const ITEM_ACTION action)
{
    return Catalog_SlotToID(
        CATALOG_ITEM_ACTIONS, action, ITEM_TRX_ACTION_INVALID);
}
