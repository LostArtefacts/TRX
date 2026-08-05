#include <trx/config/common.h>

#include <trx/config/file.h>
#include <trx/config/override.h>
#include <trx/config/priv.h>
#include <trx/config/section.h>
#include <trx/config/value.h>
#include <trx/config/vars.h>
#include <trx/core/dynamic_enum.h>
#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/core/vector.h>
#include <trx/debug.h>
#include <trx/game/game_flow/vars.h>
#include <trx/game/game_strings/entries.h>

#include <stdio.h>
#include <string.h>

// In-memory list of pointers to config options hidden by the game flow.
static VECTOR *m_HiddenOptions = nullptr;

static EVENT_MANAGER *m_EventManager = nullptr;

// Where Config_Read() was pointed, so Config_Write() knows where to put it
// back. The config module's own business, not a setting the player has.
static char *m_DefaultPath = nullptr;
static char *m_EnforcedPath = nullptr;
static bool m_Loaded = false;

static void M_FreeStringOptionValues(void)
{
    const CONFIG_OPTION *option = Config_GetOptionMap();
    while (option != nullptr && option->target != nullptr) {
        if (option->type == TVT_STRING || option->type == TVT_DYNAMIC_ENUM) {
            Memory_Free(*(char **)option->target);
        }
        option++;
    }
}

__attribute__((constructor)) static void M_Init(void)
{
    m_EventManager = EventManager_Create();
}

__attribute__((destructor)) static void M_Shutdown(void)
{
    EventManager_Free(m_EventManager);
    m_EventManager = nullptr;

    M_FreeStringOptionValues();
    Memory_FreePointer(&m_DefaultPath);
    Memory_FreePointer(&m_EnforcedPath);

    if (m_HiddenOptions != nullptr) {
        Vector_Free(m_HiddenOptions);
        m_HiddenOptions = nullptr;
    }
    ConfigOverride_Shutdown();
}

static bool M_RestoreOptionDefault(const void *const target, const bool force)
{
    if (target == nullptr) {
        return false;
    }
    const CONFIG_OPTION *option = Config_GetOption(target);
    if (option == nullptr) {
        return false;
    }
    if (!force && Config_IsOptionEnforced(target)) {
        return false;
    }
    // The string default is stored inline, so it is not a `char **` that
    // Value_CopyPtr could read; copy it here. The free-after-allocate order (so
    // subscribers see the pointer move) lives in Value_CopyPtr for the rest.
    if (option->type == TVT_STRING || option->type == TVT_DYNAMIC_ENUM) {
        char **const p = (char **)option->target;
        const char *const def = (const char *)option->default_value;
        char *const old = *p;
        *p = def != nullptr ? Memory_DupStr(def) : nullptr;
        Memory_Free(old);
        return true;
    }
    Value_CopyPtr(option->type, (void *)option->target, option->default_value);
    return true;
}

// The extra argument the value parse and format calls need: the EnumMap name
// for an enum, the dynamic enum registry token for a dynamic enum, nothing
// else.
static const void *M_ValueParam(const CONFIG_OPTION *const option)
{
    return option->type == TVT_DYNAMIC_ENUM ? option->target : option->param;
}

// The two presentations Value_Format does not carry, because each reaches past
// the stored value: a bool as a localized on/off, and a float as a percentage.
static const char *M_FormatBoolHuman(const bool value)
{
    return value ? GS("general/misc/on") : GS("general/misc/off");
}

static const char *M_FormatFloatPercent(const float value)
{
    return String_FormatStatic("%.0f%%", value);
}

static bool M_SetOptionValueFromString(
    const CONFIG_OPTION *const option, const char *const new_value,
    const bool force)
{
    ASSERT(option != nullptr);
    ASSERT(option->target != nullptr);
    if (!force && Config_IsOptionEnforced(option->target)) {
        return false;
    }
    TRX_VALUE parsed;
    if (!Value_Parse(option->type, M_ValueParam(option), new_value, &parsed)) {
        return false;
    }
    if (option->type == TVT_STRING || option->type == TVT_DYNAMIC_ENUM) {
        Value_CopyPtr(option->type, (void *)option->target, &parsed.as_str);
        return true;
    }
    if (option->percent) {
        parsed.as_num /= 100.0;
    }
    return Value_WritePtr(option->type, (void *)option->target, &parsed)
        == nullptr;
}

void Config_ApplyDefaultSettings(void)
{
    const CONFIG_OPTION *option = Config_GetOptionMap();
    while (option->target != nullptr) {
        Config_RestoreOptionDefault(option->target);
        option++;
    }
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
    ConfigOverride_Clear();

    LOG_DEBUG("Reading config");
    LOG_DEBUG("  default_path=%s", m_DefaultPath);
    LOG_DEBUG("  enforced_path=%s", m_EnforcedPath);
    if (m_HiddenOptions == nullptr) {
        m_HiddenOptions = Vector_Create(sizeof(void *));
    } else {
        Vector_ClearRealloc(m_HiddenOptions);
    }
    const CONFIG_IO_ARGS args = {
        .default_path = m_DefaultPath,
        .enforced_path = m_EnforcedPath,
        .action = &Config_LoadFromJSON,
        .hidden_targets = m_HiddenOptions,
    };
    const bool result = ConfigFile_Read(&args);
    if (result) {
        LOG_DEBUG("Config loaded");
    } else {
        LOG_WARNING("Errors while loading config");
    }
    Config_Sanitize();
    g_SavedConfig = g_Config;
    return result;
}

bool Config_Update(void)
{
    Config_Sanitize();
    // A section's data is its own module's, so nothing here can see that it
    // moved; the module that moved it says so, and that report is spent here.
    const bool section_changed = Config_Section_TakeChanged();
    if (memcmp(&g_Config, &g_SavedConfig, sizeof(CONFIG)) == 0
        && !section_changed) {
        return false;
    }

    if (m_EventManager != nullptr) {
        const EVENT event = {
            .name = "change",
            .sender = nullptr,
            .data = nullptr,
        };
        EventManager_Fire(m_EventManager, &event);
    }
    g_SavedConfig = g_Config;
    return true;
}

bool Config_IsLoaded(void)
{
    return m_Loaded;
}

bool Config_Write(void)
{
    ASSERT(m_DefaultPath != nullptr);
    const CONFIG_IO_ARGS args = {
        .default_path = m_DefaultPath,
        .enforced_path = m_EnforcedPath,
        .action = &Config_DumpToJSON,
    };
    return ConfigFile_Write(&args);
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

const CONFIG_OPTION *Config_GetOption(const void *const target)
{
    const CONFIG_OPTION *option = Config_GetOptionMap();
    if (option == nullptr) {
        return nullptr;
    }
    while (option->target != nullptr) {
        if (option->target == target) {
            return option;
        }
        option++;
    }
    return nullptr;
}

bool Config_PushOptionOverride(
    const void *const target, const void *const value)
{
    ASSERT(target != nullptr);
    const CONFIG_OPTION *const option = Config_GetOption(target);
    return option != nullptr && ConfigOverride_Push(option, value);
}

bool Config_PopOptionOverride(const void *const target)
{
    ASSERT(target != nullptr);
    const CONFIG_OPTION *const option = Config_GetOption(target);
    return option != nullptr && ConfigOverride_Pop(option);
}

bool Config_IsOptionOverridden(const void *const target)
{
    ASSERT(target != nullptr);
    const CONFIG_OPTION *const option = Config_GetOption(target);
    return option != nullptr && ConfigOverride_IsOverridden(option);
}

const void *Config_GetOptionValueForSave(const CONFIG_OPTION *const option)
{
    ASSERT(option != nullptr);
    return ConfigOverride_GetBaseValuePtr(option);
}

bool Config_IsOptionEnforced(const void *const target)
{
    return Config_IsOptionOverridden(target);
}

bool Config_IsOptionHidden(const void *const target)
{
    return m_HiddenOptions != nullptr
        && Vector_Contains(m_HiddenOptions, &target);
}

bool Config_IsOptionAtDefault(const void *const target)
{
    const CONFIG_OPTION *option = Config_GetOption(target);
    if (target == nullptr) {
        return true;
    }
    // A string default is stored inline as the string, not behind a pointer to
    // one, so it cannot be addressed the way Value_EqualPtr addresses the live
    // value; compare it here.
    if (option->type == TVT_STRING || option->type == TVT_DYNAMIC_ENUM) {
        const char *const cur = *(char **)option->target;
        const char *const def = (const char *)option->default_value;
        if (cur == nullptr || def == nullptr) {
            return cur == def;
        }
        return strcmp(cur, def) == 0;
    }
    return Value_EqualPtr(option->type, option->target, option->default_value);
}

bool Config_RestoreOptionDefault(const void *const target)
{
    return M_RestoreOptionDefault(target, false);
}

bool Config_RestoreOptionDefaultForce(const void *const target)
{
    return M_RestoreOptionDefault(target, true);
}

const char *Config_GetOptionValueAsString(
    const CONFIG_OPTION *const option, const bool human_readable)
{
    if (option == nullptr) {
        return nullptr;
    }
    if (human_readable && option->type == TVT_BOOL) {
        return M_FormatBoolHuman(*(bool *)option->target);
    }
    if (option->percent) {
        return M_FormatFloatPercent((*(float *)option->target) * 100.0f);
    }
    TRX_VALUE value;
    Value_ReadPtr(option->type, option->target, &value);
    return Value_Format(
        option->type, M_ValueParam(option), &value, human_readable);
}

const char *Config_GetOptionTitle(const CONFIG_OPTION *const opt)
{
    if (opt == nullptr || opt->name == nullptr) {
        return nullptr;
    }
    return GameString_Get(String_FormatStatic("settings/%s/title", opt->name));
}

const char *Config_GetOptionDescription(const CONFIG_OPTION *const opt)
{
    if (opt == nullptr || opt->name == nullptr) {
        return nullptr;
    }
    return GameString_Get(
        String_FormatStatic("settings/%s/description", opt->name));
}

char *Config_NormalizeOptionValueString(
    const CONFIG_OPTION *const option, const char *const value,
    const bool human_readable)
{
    if (option == nullptr) {
        return Memory_DupStr(value != nullptr ? value : "");
    }

    const char *const input = value != nullptr ? value : "";

    TRX_VALUE parsed;
    if (!Value_Parse(option->type, M_ValueParam(option), input, &parsed)) {
        return Memory_DupStr(input);
    }
    if (human_readable && option->type == TVT_BOOL) {
        return Memory_DupStr(M_FormatBoolHuman(parsed.as_bool));
    }
    if (option->percent) {
        return Memory_DupStr(M_FormatFloatPercent(parsed.as_num));
    }
    return Memory_DupStr(Value_Format(
        option->type, M_ValueParam(option), &parsed, human_readable));
}

bool Config_SetOptionValueFromString(
    const CONFIG_OPTION *const option, const char *const new_value)
{
    return M_SetOptionValueFromString(option, new_value, false);
}

bool Config_SetOptionValueFromStringForce(
    const CONFIG_OPTION *const option, const char *const new_value)
{
    return M_SetOptionValueFromString(option, new_value, true);
}
