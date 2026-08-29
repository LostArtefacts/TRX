#include <trx/game/catalog/manager.h>
#include <trx/game/items.h>

ITEM_ACTION_SLOT ItemAction_IDToSlot(const ITEM_ACTION_ID id)
{
    return Catalog_IDToSlot(CATALOG_ITEM_ACTIONS, id, ITEM_ACTION_INVALID);
}

ITEM_ACTION_ID ItemAction_SlotToID(const ITEM_ACTION_SLOT slot)
{
    return Catalog_SlotToID(CATALOG_ITEM_ACTIONS, slot, NO_CATALOG_ID);
}
