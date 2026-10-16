#include <trx/game/ui/menu.h>

static OBJECT_ID m_RequestedItem = NO_OBJECT;

void UI_RequestItem(const OBJECT_ID object_id)
{
    m_RequestedItem = object_id;
}

OBJECT_ID UI_TakeRequestedItem(void)
{
    const OBJECT_ID object_id = m_RequestedItem;
    m_RequestedItem = NO_OBJECT;
    return object_id;
}
