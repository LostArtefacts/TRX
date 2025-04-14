#include "game/ui2/events.h"

#include "config/common.h"
#include "debug.h"
#include "game/ui2/common.h"

static EVENT_MANAGER *m_EventManager = nullptr;

void UI2_InitEvents(void)
{
    m_EventManager = EventManager_Create();
}

void UI2_ShutdownEvents(void)
{
    EventManager_Free(m_EventManager);
    m_EventManager = nullptr;
}

int32_t UI2_Subscribe(
    const char *const event_name, const void *const sender,
    const EVENT_LISTENER listener, void *const user_data)
{
    ASSERT(m_EventManager != nullptr);
    return EventManager_Subscribe(
        m_EventManager, event_name, sender, listener, user_data);
}

void UI2_Unsubscribe(const int32_t listener_id)
{
    if (m_EventManager != nullptr) {
        EventManager_Unsubscribe(m_EventManager, listener_id);
    }
}

void UI2_FireEvent(const EVENT event)
{
    if (m_EventManager != nullptr) {
        EventManager_Fire(m_EventManager, &event);
    }
}
