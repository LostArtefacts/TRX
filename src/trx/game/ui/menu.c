#include <trx/game/ui/menu.h>

static OBJECT_ID m_RequestedItem = NO_OBJECT;
static bool m_SaveRequested = false;

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

void UI_RequestSave(const bool requested)
{
    m_SaveRequested = requested;
}

bool UI_IsSaveRequested(void)
{
    return m_SaveRequested;
}
