#include "config/common.h"

#include "config/file.h"
#include "config/priv.h"
#include "config/vars.h"
#include "debug.h"
#include "game/shell.h"
#include "vector.h"

typedef enum {
    CFT_DEFAULT,
    CFT_ENFORCED,
} CONFIG_FILE_TYPE;

// In-memory list of pointers to config options enforced by the game flow.
static VECTOR *m_EnforcedOptions = nullptr;

static EVENT_MANAGER *m_EventManager = nullptr;

static const char *M_GetPath(CONFIG_FILE_TYPE file_type);

static const char *M_GetPath(const CONFIG_FILE_TYPE file_type)
{
    return file_type == CFT_DEFAULT ? Shell_GetConfigPath()
                                    : Shell_GetGameFlowPath();
}

void Config_Init(void)
{
    m_EventManager = EventManager_Create();
}

void Config_Shutdown(void)
{
    EventManager_Free(m_EventManager);
    m_EventManager = nullptr;

    if (m_EnforcedOptions != nullptr) {
        Vector_Free(m_EnforcedOptions);
        m_EnforcedOptions = nullptr;
    }
}

bool Config_Read(void)
{
    if (m_EnforcedOptions == nullptr) {
        m_EnforcedOptions = Vector_Create(sizeof(void *));
    } else {
        Vector_Clear(m_EnforcedOptions);
    }
    const CONFIG_IO_ARGS args = {
        .default_path = M_GetPath(CFT_DEFAULT),
        .enforced_path = M_GetPath(CFT_ENFORCED),
        .action = &Config_LoadFromJSON,
        .enforced_targets = m_EnforcedOptions,
    };
    const bool result = ConfigFile_Read(&args);
    if (result) {
        Config_Sanitize();
        g_SavedConfig = g_Config;
    }
    return result;
}

bool Config_Write(void)
{
    Config_Sanitize();
    const CONFIG_IO_ARGS args = {
        .default_path = M_GetPath(CFT_DEFAULT),
        .enforced_path = M_GetPath(CFT_ENFORCED),
        .action = &Config_DumpToJSON,
    };
    const bool updated = ConfigFile_Write(&args);
    if (updated) {
        if (m_EventManager != nullptr) {
            const EVENT event = {
                .name = "change",
                .sender = nullptr,
                .data = nullptr,
            };
            EventManager_Fire(m_EventManager, &event);
        }
        g_SavedConfig = g_Config;
    }
    return updated;
}

int32_t Config_SubscribeChanges(
    const EVENT_LISTENER listener, void *const user_data)
{
    ASSERT(m_EventManager != nullptr);
    return EventManager_Subscribe(
        m_EventManager, "change", nullptr, listener, user_data);
}

void Config_UnsubscribeChanges(const int32_t listener_id)
{
    ASSERT(m_EventManager != nullptr);
    EventManager_Unsubscribe(m_EventManager, listener_id);
}

bool Config_IsOptionEnforced(const void *const target)
{
    return m_EnforcedOptions != nullptr
        && Vector_Contains(m_EnforcedOptions, &target);
}
