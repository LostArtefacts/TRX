#include <trx/game/ui/menu.h>

#include <trx/game/lua/events.h>

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

void UI_ShowPickup(const OBJECT_ID object_id)
{
    LUA_FireEventInt32(LUA_EVENT_SHOW_PICKUP, object_id);
}
