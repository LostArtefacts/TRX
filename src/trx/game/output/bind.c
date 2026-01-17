#include <trx/game/output/bind.h>

#include <trx/game/items.h>

#include <string.h>

static OUTPUT_ITEM_BIND m_ItemBindings[MAX_ITEMS] = {};

void Output_Bind_ResetItems(void)
{
    memset(m_ItemBindings, 0, sizeof(m_ItemBindings));
}

OUTPUT_ITEM_BIND *Output_Bind_GetItem(const ITEM *const item)
{
    return &m_ItemBindings[Item_GetIndex(item)];
}
