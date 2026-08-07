#include <trx/config/common.h>

#include <trx/config/file.h>
#include <trx/config/priv.h>
#include <trx/config/registry.h>
#include <trx/config/section.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/vector.h>
#include <trx/debug.h>

#include <string.h>

static EVENT_MANAGER *m_EventManager = nullptr;

// What has moved since the last report was taken, and whether any of it is the
// player's own doing.
static VECTOR *m_Pending = nullptr;
static bool m_PendingPersist = false;

// Where Config_Read() was pointed, so Config_Write() knows where to put it
// back. The config module's own business, not a setting the player has.
static char *m_DefaultPath = nullptr;
static char *m_EnforcedPath = nullptr;
static bool m_Loaded = false;
static bool m_FileFound = false;

__attribute__((constructor)) static void M_Init(void)
{
    m_EventManager = EventManager_Create();
}

__attribute__((destructor)) static void M_Shutdown(void)
{
    EventManager_Free(m_EventManager);
    m_EventManager = nullptr;
    if (m_Pending != nullptr) {
        Vector_Free(m_Pending);
        m_Pending = nullptr;
    }
    Memory_FreePointer(&m_DefaultPath);
    Memory_FreePointer(&m_EnforcedPath);
}

// What a type converts as. An enum is int32 storage that happens to spell its
// values, and a dynamic enum is string storage that does; both convert as the
// storage they are.
static TRX_VALUE_TYPE M_StorageType(const TRX_VALUE_TYPE type)
{
    switch (type) {
    case TVT_ENUM:
        return TVT_S32;
    case TVT_DYNAMIC_ENUM:
        return TVT_STRING;
    default:
        return type;
    }
}

// The value as the option's own type. A caller states a value in the shape its
// own C expression had - an enumeration constant is an integer, a whole number
// written for a float setting is an integer, an outfit name is a string - and
// the option says what it was meant as.
static bool M_AdaptValue(
    const CONFIG_OPTION *const option, TRX_VALUE *const value)
{
    const TRX_VALUE_TYPE type = option->value.type;
    if (value->type == type) {
        return true;
    }
    // Carried over whole rather than through as_int: the carrier is a union,
    // and which member a value rides in is the type's business.
    TRX_VALUE from = *value;
    from.type = M_StorageType(value->type);
    TRX_VALUE out;
    if (!Value_Coerce(M_StorageType(type), &from, &out)) {
        return false;
    }
    out.type = type;
    *value = out;
    return true;
}

void Config_ReportChange(const CONFIG_OPTION *const option, const bool persist)
{
    m_PendingPersist |= persist;
    if (m_Pending == nullptr) {
        m_Pending = Vector_Create(sizeof(const CONFIG_OPTION *));
    }
    for (int32_t i = 0; i < m_Pending->count; i++) {
        if (*(const CONFIG_OPTION **)Vector_Get(m_Pending, i) == option) {
            return;
        }
    }
    Vector_Add(m_Pending, &option);
}

bool Config_Read(
    const char *const default_path, const char *const enforced_path)
{
    // Always initialize the config, even if the file is missing, so that
    // the game can interact with these properties.
    Memory_FreePointer(&m_DefaultPath);
    Memory_FreePointer(&m_EnforcedPath);
    m_DefaultPath = Memory_DupStr(default_path);
    m_EnforcedPath = Memory_DupStr(enforced_path);
    m_Loaded = true;
    Config_ClearHolds();

    LOG_DEBUG("Reading config");
    LOG_DEBUG("  default_path=%s", m_DefaultPath);
    LOG_DEBUG("  enforced_path=%s", m_EnforcedPath);

    const bool result = ConfigFile_Read(m_DefaultPath, m_EnforcedPath);
    m_FileFound = ConfigFile_WasFound();
    if (result) {
        LOG_DEBUG("Config loaded");
    } else {
        LOG_WARNING("Errors while loading config");
    }

    Config_LoadFromJSON(ConfigFile_GetRoot());
    Config_Sanitize();
    // What the file held was never a change: it is where the settings start.
    Config_DiscardPendingChanges();
    return result;
}

bool Config_Update(void)
{
    Config_Sanitize();
    // A section's data is its own module's, so nothing here can see that it
    // moved; the module that moved it says so, and that report is spent here.
    const bool section_changed = Config_Section_TakeChanged();
    if ((m_Pending == nullptr || m_Pending->count == 0) && !section_changed) {
        return false;
    }

    // The report is taken before it is spent, not after: a listener that moves
    // a setting of its own comes back through here, and what it moved is a
    // report of its own rather than this one told again.
    VECTOR *const taken = m_Pending;
    const CONFIG_CHANGE change = {
        // A rebound key is the player's doing as much as a setting is.
        .persist = m_PendingPersist || section_changed,
        .options = taken != nullptr ? Vector_GetData(taken) : nullptr,
        .count = taken != nullptr ? taken->count : 0,
    };
    m_Pending = nullptr;
    m_PendingPersist = false;

    if (m_EventManager != nullptr) {
        const EVENT event = {
            .name = "change",
            .sender = nullptr,
            .data = (void *)&change,
        };
        EventManager_Fire(m_EventManager, &event);
    }
    if (taken != nullptr) {
        Vector_Free(taken);
    }
    return true;
}

void Config_DiscardPendingChanges(void)
{
    if (m_Pending != nullptr) {
        Vector_Clear(m_Pending);
    }
    m_PendingPersist = false;
}

bool Config_IsLoaded(void)
{
    return m_Loaded;
}

bool Config_IsFirstRun(void)
{
    return m_Loaded && !m_FileFound;
}

bool Config_Write(void)
{
    ASSERT(m_DefaultPath != nullptr);
    return ConfigFile_Write(m_DefaultPath, &Config_DumpToJSON);
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

bool Config_Change_HasMirror(
    const CONFIG_CHANGE *const change, const void *const mirror)
{
    if (change == nullptr) {
        return false;
    }
    for (int32_t i = 0; i < change->count; i++) {
        if (change->options[i]->mirror == mirror) {
            return true;
        }
    }
    return false;
}

const char *Config_ResolveOptionName(const char *const option_name)
{
    const char *const dot = strrchr(option_name, '.');
    if (dot != nullptr) {
        return dot + 1;
    }
    return option_name;
}

bool Config_SetValue(const void *const mirror, TRX_VALUE value)
{
    CONFIG_OPTION *const option = Config_FindOptionByMirror(mirror);
    if (option == nullptr || !M_AdaptValue(option, &value)) {
        return false;
    }
    Config_Option_Write(option, &value);
    return true;
}

bool Config_PushHold(
    const void *const mirror, TRX_VALUE value, const CONFIG_HOLD_SOURCE source)
{
    CONFIG_OPTION *const option = Config_FindOptionByMirror(mirror);
    if (option == nullptr || !M_AdaptValue(option, &value)) {
        return false;
    }
    return Config_Option_PushHold(option, &value, source);
}

bool Config_PopHold(const void *const mirror)
{
    CONFIG_OPTION *const option = Config_FindOptionByMirror(mirror);
    return option != nullptr && Config_Option_PopHold(option);
}
