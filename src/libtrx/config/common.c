#include "config/common.h"

#include "config/file.h"

#include <assert.h>

EVENT_MANAGER *m_EventManager = NULL;

void Config_Init(void)
{
    m_EventManager = EventManager_Create();
}

void Config_Shutdown(void)
{
    EventManager_Free(m_EventManager);
    m_EventManager = NULL;
}

bool Config_Read(void)
{
    const bool result = ConfigFile_Read(Config_GetPath(), &Config_LoadFromJSON);
    if (result) {
        Config_Sanitize();
        Config_ApplyChanges();
    }
    return result;
}

bool Config_Write(void)
{
    Config_Sanitize();
    const bool updated = ConfigFile_Write(Config_GetPath(), &Config_DumpToJSON);
    if (updated) {
        Config_ApplyChanges();
        if (m_EventManager != NULL) {
            const EVENT event = {
                .name = "write",
                .sender = NULL,
                .data = NULL,
            };
            EventManager_Fire(m_EventManager, &event);
        }
    }
    return updated;
}

int32_t Config_SubscribeChanges(
    const EVENT_LISTENER listener, void *const user_data)
{
    assert(m_EventManager != NULL);
    return EventManager_Subscribe(
        m_EventManager, "write", NULL, listener, user_data);
}

void Config_UnsubscribeChanges(const int32_t listener_id)
{
    assert(m_EventManager != NULL);
    return EventManager_Unsubscribe(m_EventManager, listener_id);
}
