#include <trx/config/common.h>

#include <trx/config/dynamic_enum.h>
#include <trx/config/file.h>
#include <trx/config/priv.h>
#include <trx/config/vars.h>
#include <trx/debug.h>
#include <trx/game/game_flow/vars.h>
#include <trx/game/shell.h>
#include <trx/memory.h>
#include <trx/strings.h>

#include <stdio.h>
#include <string.h>

// In-memory list of pointers to config options enforced by the game flow.
static VECTOR *m_EnforcedOptions = nullptr;
// In-memory list of pointers to config options hidden by the game flow.
static VECTOR *m_HiddenOptions = nullptr;

static EVENT_MANAGER *m_EventManager = nullptr;

static void M_FreeStringOptionValues(void)
{
    const CONFIG_OPTION *option = Config_GetOptionMap();
    while (option != nullptr && option->target != nullptr) {
        if (option->type == COT_STRING || option->type == COT_DYNAMIC_ENUM) {
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
    Memory_FreePointer(&g_Config.default_path);
    Memory_FreePointer(&g_Config.enforced_path);

    if (m_EnforcedOptions != nullptr) {
        Vector_Free(m_EnforcedOptions);
        m_EnforcedOptions = nullptr;
    }
    if (m_HiddenOptions != nullptr) {
        Vector_Free(m_HiddenOptions);
        m_HiddenOptions = nullptr;
    }
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
    Memory_FreePointer(&g_Config.default_path);
    Memory_FreePointer(&g_Config.enforced_path);
    g_Config.default_path = Memory_DupStr(default_path);
    g_Config.enforced_path = Memory_DupStr(enforced_path);
    g_Config.loaded = true;

    LOG_DEBUG("Reading config");
    LOG_DEBUG("  default_path=%s", g_Config.default_path);
    LOG_DEBUG("  enforced_path=%s", g_Config.enforced_path);
    if (m_EnforcedOptions == nullptr) {
        m_EnforcedOptions = Vector_Create(sizeof(void *));
    } else {
        Vector_ClearRealloc(m_EnforcedOptions);
    }
    if (m_HiddenOptions == nullptr) {
        m_HiddenOptions = Vector_Create(sizeof(void *));
    } else {
        Vector_ClearRealloc(m_HiddenOptions);
    }
    const CONFIG_IO_ARGS args = {
        .default_path = g_Config.default_path,
        .enforced_path = g_Config.enforced_path,
        .action = &Config_LoadFromJSON,
        .enforced_targets = m_EnforcedOptions,
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
    if (memcmp(&g_Config, &g_SavedConfig, sizeof(CONFIG)) == 0) {
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
    g_Config.dirty = false;
    g_SavedConfig = g_Config;
    return true;
}

bool Config_Write(void)
{
    ASSERT(g_Config.default_path != nullptr);
    const CONFIG_IO_ARGS args = {
        .default_path = g_Config.default_path,
        .enforced_path = g_Config.enforced_path,
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

bool Config_IsOptionEnforced(const void *const target)
{
    return m_EnforcedOptions != nullptr
        && Vector_Contains(m_EnforcedOptions, &target);
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
    switch (option->type) {
    case COT_BOOL:
    case COT_INVERTED_BOOL:
        return *(bool *)option->target == *(bool *)option->default_value;
    case COT_INT32:
        return *(int32_t *)option->target == *(int32_t *)option->default_value;
    case COT_FLOAT:
    case COT_FLOAT_PERCENT:
        return *(float *)option->target == *(float *)option->default_value;
    case COT_DOUBLE:
        return *(double *)option->target == *(double *)option->default_value;
    case COT_RGB888: {
        const RGB_888 cur = *(RGB_888 *)option->target;
        const RGB_888 def = *(RGB_888 *)option->default_value;
        return cur.r == def.r && cur.g == def.g && cur.b == def.b;
    }
    case COT_ENUM:
        return *(int32_t *)option->target == *(int32_t *)option->default_value;
        break;
    case COT_STRING:
    case COT_DYNAMIC_ENUM: {
        const char *const cur = *(char **)option->target;
        const char *const def = (const char *)option->default_value;
        if (cur == nullptr && def == nullptr) {
            return true;
        }
        if (cur == nullptr || def == nullptr) {
            return false;
        }
        return strcmp(cur, def) == 0;
    }
    }
    return true;
}

bool Config_RestoreOptionDefault(const void *const target)
{
    const CONFIG_OPTION *option = Config_GetOption(target);
    if (target == nullptr) {
        return false;
    }
    switch (option->type) {
    case COT_BOOL:
    case COT_INVERTED_BOOL:
        *(bool *)option->target = *(bool *)option->default_value;
        return true;
    case COT_INT32:
        *(int32_t *)option->target = *(int32_t *)option->default_value;
        return true;
    case COT_FLOAT:
    case COT_FLOAT_PERCENT:
        *(float *)option->target = *(float *)option->default_value;
        return true;
    case COT_DOUBLE:
        *(double *)option->target = *(double *)option->default_value;
        return true;
    case COT_RGB888:
        *(RGB_888 *)option->target = *(RGB_888 *)option->default_value;
        return true;
    case COT_ENUM:
        *(int32_t *)option->target = *(int32_t *)option->default_value;
        return true;
    case COT_STRING:
    case COT_DYNAMIC_ENUM: {
        char **const p = (char **)option->target;
        const char *const def = (const char *)option->default_value;
        char *const old = *p;
        *p = def != nullptr ? Memory_DupStr(def) : nullptr;
        // VERY IMPORTANT: free the memory AFTER we allocate, so that we force
        // the pointer to get a different macro, so that change subscribers
        // can see the string has changed by comparing just the pointers.
        Memory_Free(old);
        return true;
    }
    }
    return false;
}

const char *Config_GetOptionValueAsString(const CONFIG_OPTION *const option)
{
    if (option == nullptr) {
        return nullptr;
    }
    switch (option->type) {
    case COT_BOOL:
        return String_FormatStatic("%d", *(bool *)option->target);
    case COT_INVERTED_BOOL:
        return String_FormatStatic("%d", !*(bool *)option->target);
    case COT_INT32:
        return String_FormatStatic("%d", *(int32_t *)option->target);
    case COT_FLOAT:
        return String_FormatStatic("%.2f", *(float *)option->target);
    case COT_FLOAT_PERCENT:
        return String_FormatStatic(
            "%.0f%%", (*(float *)option->target) * 100.0f);
    case COT_DOUBLE:
        return String_FormatStatic("%.2f", *(double *)option->target);
    case COT_ENUM:
        return String_FormatStatic(
            "%s", EnumMap_ToString(option->param, *(int32_t *)option->target));
    case COT_RGB888: {
        const RGB_888 *const color = option->target;
        return String_FormatStatic(
            "%02hhx%02hhx%02hhx", color->r, color->g, color->b);
    }
    case COT_STRING:
    case COT_DYNAMIC_ENUM:
        return String_FormatStatic(
            "%s",
            *(char **)option->target != nullptr ? *(char **)option->target
                                                : "");
    default:
        return nullptr;
    }
}

bool Config_SetOptionValueFromString(
    const CONFIG_OPTION *const option, const char *const new_value)
{
    ASSERT(option != nullptr);
    ASSERT(option->target != nullptr);
    switch (option->type) {
    case COT_BOOL:
        if (String_Match(new_value, "^(on|true|1)$")) {
            *(bool *)option->target = true;
            return true;
        } else if (String_Match(new_value, "^(off|false|0)$")) {
            *(bool *)option->target = false;
            return true;
        }
        break;

    case COT_INVERTED_BOOL:
        if (String_Match(new_value, "^(on|true|1)$")) {
            *(bool *)option->target = false;
            return true;
        } else if (String_Match(new_value, "^(off|false|0)$")) {
            *(bool *)option->target = true;
            return true;
        }
        break;

    case COT_INT32: {
        int32_t new_value_typed;
        if (sscanf(new_value, "%d", &new_value_typed) == 1) {
            *(int32_t *)option->target = new_value_typed;
            return true;
        }
        break;
    }

    case COT_FLOAT: {
        float new_value_typed;
        if (sscanf(new_value, "%f", &new_value_typed) == 1) {
            *(float *)option->target = new_value_typed;
            return true;
        }
        break;
    }

    case COT_FLOAT_PERCENT: {
        float new_value_typed;
        if (sscanf(new_value, "%f", &new_value_typed) == 1) {
            *(float *)option->target = new_value_typed / 100.0f;
            return true;
        }
        break;
    }

    case COT_DOUBLE: {
        double new_value_typed;
        if (sscanf(new_value, "%lf", &new_value_typed) == 1) {
            *(double *)option->target = new_value_typed;
            return true;
        }
        break;
    }

    case COT_ENUM: {
        const int32_t new_value_typed =
            EnumMap_Get(option->param, new_value, -1);
        if (new_value_typed != -1) {
            *(int32_t *)option->target = new_value_typed;
            return true;
        }
        break;
    }

    case COT_RGB888: {
        uint8_t r, g, b;
        if (sscanf(new_value, "%02hhx%02hhx%02hhx", &r, &g, &b) == 3) {
            RGB_888 *const color = (RGB_888 *)option->target;
            color->r = r;
            color->g = g;
            color->b = b;
            return true;
        }
        break;
    }

    case COT_STRING:
    case COT_DYNAMIC_ENUM: {
        if (option->type == COT_DYNAMIC_ENUM
            && !Config_DynamicEnum_IsValidValue(option, new_value)) {
            return false;
        }
        char **const p = (char **)option->target;
        char *const old = *p;
        *p = new_value != nullptr ? Memory_DupStr(new_value) : nullptr;
        // VERY IMPORTANT: free the memory AFTER we allocate, so that we force
        // the pointer to get a different macro, so that change subscribers
        // can see the string has changed by comparing just the pointers.
        Memory_Free(old);
        return true;
    }
    }

    return false;
}
