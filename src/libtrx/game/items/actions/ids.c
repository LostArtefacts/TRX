#include "game/catalog.h"
#include "game/items.h"

ITEM_ACTION Item_ActionToGameID(const ITEM_TRX_ACTION action)
{
    ITEM_ACTION out;
    if (Catalog_EnumToGameID(CATALOG_ITEM_ACTIONS, action, &out)) {
        return out;
    }
    return ITEM_ACTION_INVALID;
}

ITEM_TRX_ACTION Item_ActionFromGameID(const ITEM_ACTION action)
{
    ITEM_TRX_ACTION out;
    if (Catalog_GameIDToEnum(CATALOG_ITEM_ACTIONS, action, &out)) {
        return out;
    }
    return ITEM_TRX_ACTION_INVALID;
}
