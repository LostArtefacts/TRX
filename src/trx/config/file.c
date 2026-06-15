#include <trx/config/file.h>

#include <trx/config/common.h>
#include <trx/core/colors.h>
#include <trx/core/filesystem.h>
#include <trx/core/json/util/file.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/core/utils.h>
#include <trx/core/vector.h>
#include <trx/debug.h>
#include <trx/game/console/history.h>

#include <stdio.h>
#include <string.h>

#define M_EMPTY_ROOT "{}"
#define M_ENFORCED_KEY "enforced_config"
#define M_HIDDEN_KEY "hidden_config"

typedef bool (*M_OPTION_VALUE_ACTION)(
    const CONFIG_OPTION *option, const void *value);

static bool M_ProcessOptionValue(
    const CONFIG_OPTION *option, const JSON_VALUE *value,
    M_OPTION_VALUE_ACTION action);
static bool M_SetOptionValue(const CONFIG_OPTION *option, const void *value);
static bool M_PushOptionOverride(
    const CONFIG_OPTION *option, const void *value);

static void M_NormalizeGymTrackStats(GYM_TRACK_STATS *const stats)
{
    GYM_TRACK_ENTRY sorted_entries[MAX_ASSAULT_TIMES] = {};
    int32_t count = 0;
    uint32_t max_attempt_num = 0;

    for (int32_t i = 0; i < MAX_ASSAULT_TIMES; i++) {
        const GYM_TRACK_ENTRY entry = stats->entries[i];
        if (entry.time == 0) {
            continue;
        }

        if (entry.attempt_num > max_attempt_num) {
            max_attempt_num = entry.attempt_num;
        }

        int32_t insert_idx = count;
        while (insert_idx > 0
               && sorted_entries[insert_idx - 1].time > entry.time) {
            sorted_entries[insert_idx] = sorted_entries[insert_idx - 1];
            insert_idx--;
        }
        sorted_entries[insert_idx] = entry;
        count++;
    }

    for (int32_t i = 0; i < MAX_ASSAULT_TIMES; i++) {
        stats->entries[i] = sorted_entries[i];
    }

    if (stats->total_attempts < max_attempt_num) {
        stats->total_attempts = max_attempt_num;
    }
}

static bool M_ReadFromJSON(
    const char *const default_path, const char *const enforced_path,
    void (*load)(JSON_OBJECT *root_obj), VECTOR *const hidden_targets)
{
    bool result = false;

    JSON_VALUE *cfg_root = JSONFile_Read(default_path);
    if (cfg_root != nullptr) {
        result = true;
    } else {
        JSON_OBJECT *const cfg_root_obj = JSON_ObjectNew();
        JSON_ObjectAppendInt(cfg_root_obj, "config_version", -1);
        cfg_root = JSON_ValueFromObject(cfg_root_obj);
    }
    JSON_VALUE *const enf_root =
        enforced_path != nullptr ? JSONFile_Read(enforced_path) : nullptr;

    JSON_OBJECT *cfg_root_obj = JSON_ValueAsObject(cfg_root);
    JSON_OBJECT *enf_root_obj = JSON_ValueAsObject(enf_root);

    // Record settings enforced by the game flow file.
    JSON_OBJECT *const enforced_config =
        JSON_ObjectGetObject(enf_root_obj, M_ENFORCED_KEY);

    // Record hidden settings from the game flow file.
    JSON_ARRAY *const hidden_config_arr =
        JSON_ObjectGetArray(enf_root_obj, M_HIDDEN_KEY);
    if (hidden_config_arr != nullptr && hidden_targets != nullptr) {
        Vector_ClearRealloc(hidden_targets);
        for (size_t i = 0; i < hidden_config_arr->length; i++) {
            const char *const name =
                JSON_ArrayGetString(hidden_config_arr, i, nullptr);
            if (name == nullptr) {
                LOG_WARNING(
                    "Expected element %d in \"%s\" to be a string", i,
                    M_HIDDEN_KEY);
                continue;
            }
            const CONFIG_OPTION *const opt = Config_GetOptionByPath(name);
            if (opt != nullptr) {
                Vector_Add(hidden_targets, &opt->target);
            }
        }
    }

    load(cfg_root_obj);

    if (enforced_config != nullptr) {
        const JSON_OBJECT_ELEMENT *elem = enforced_config->start;
        while (elem != nullptr) {
            const char *const name = elem->name->string;
            const CONFIG_OPTION *const opt = Config_GetOptionByPath(name);
            if (opt != nullptr) {
                if (!M_ProcessOptionValue(
                        opt, elem->value, M_PushOptionOverride)) {
                    LOG_WARNING("Failed to enforce config option '%s'", name);
                }
            }
            elem = elem->next;
        }
    }

    if (cfg_root) {
        JSON_ValueFree(cfg_root);
    }
    if (enf_root) {
        JSON_ValueFree(enf_root);
    }

    return result;
}

static bool M_SetOptionValue(
    const CONFIG_OPTION *const option, const void *const value)
{
    ASSERT(option != nullptr);
    ASSERT(value != nullptr);

    switch (option->type) {
    case COT_BOOL:
        *(bool *)option->target = *(const bool *)value;
        return true;
    case COT_INT32:
        *(int32_t *)option->target = *(const int32_t *)value;
        return true;
    case COT_FLOAT:
    case COT_FLOAT_PERCENT:
        *(float *)option->target = *(const float *)value;
        return true;
    case COT_DOUBLE:
        *(double *)option->target = *(const double *)value;
        return true;
    case COT_ENUM:
        *(int *)option->target = *(const int *)value;
        return true;
    case COT_STRING:
    case COT_DYNAMIC_ENUM: {
        char **const p = (char **)option->target;
        char *const old = *p;
        const char *const new_value = *(const char *const *)value;
        *p = new_value != nullptr ? Memory_DupStr(new_value) : nullptr;
        Memory_Free(old);
        return true;
    }
    case COT_RGB888:
        *(RGB_888 *)option->target = *(const RGB_888 *)value;
        return true;
    }
    return false;
}

static bool M_PushOptionOverride(
    const CONFIG_OPTION *const option, const void *const value)
{
    ASSERT(option != nullptr);
    return Config_PushOptionOverride(option->target, value);
}

static bool M_ProcessOptionValue(
    const CONFIG_OPTION *const option, const JSON_VALUE *const value,
    const M_OPTION_VALUE_ACTION action)
{
    ASSERT(option != nullptr);
    ASSERT(action != nullptr);

    switch (option->type) {
    case COT_BOOL: {
        const bool parsed =
            JSON_ValueGetBool(value, *(bool *)option->default_value);
        return action(option, &parsed);
    }

    case COT_INT32: {
        int32_t parsed = *(int32_t *)option->default_value;
        if (value != nullptr && value->type == JSON_TYPE_NUMBER) {
            parsed = JSON_ValueGetInt(value, *(int32_t *)option->default_value);
        } else if (value != nullptr && value->type == JSON_TYPE_STRING) {
            String_ParseInteger(JSON_ValueGetString(value, ""), &parsed);
        }
        return action(option, &parsed);
    }

    case COT_FLOAT:
    case COT_FLOAT_PERCENT: {
        const float parsed =
            JSON_ValueGetDouble(value, *(float *)option->default_value);
        return action(option, &parsed);
    }

    case COT_DOUBLE: {
        const double parsed =
            JSON_ValueGetDouble(value, *(double *)option->default_value);
        return action(option, &parsed);
    }

    case COT_ENUM: {
        const char *const str_value = JSON_ValueGetString(value, nullptr);
        const int parsed = str_value != nullptr
            ? EnumMap_Get(
                  option->param, str_value, *(int *)option->default_value)
            : *(int *)option->default_value;
        return action(option, &parsed);
    }

    case COT_STRING:
    case COT_DYNAMIC_ENUM: {
        const char *const parsed =
            JSON_ValueGetString(value, (const char *)option->default_value);
        return action(option, &parsed);
    }

    case COT_RGB888: {
        RGB_888 parsed;
        bool success = false;
        if (value != nullptr && value->type == JSON_TYPE_NUMBER) {
            const uint32_t rgb_value =
                JSON_ValueGetInt(value, JSON_INVALID_NUMBER);
            ASSERT(rgb_value != JSON_INVALID_NUMBER);
            parsed.r = (rgb_value >> 0) & 0xFF;
            parsed.g = (rgb_value >> 8) & 0xFF;
            parsed.b = (rgb_value >> 16) & 0xFF;
            success = true;
        } else if (value != nullptr && value->type == JSON_TYPE_STRING) {
            const char *str_value =
                JSON_ValueGetString(value, JSON_INVALID_STRING);
            ASSERT(str_value != JSON_INVALID_STRING);
            success = String_ParseRGB888(str_value, &parsed);
        }
        if (!success) {
            parsed = *(RGB_888 *)option->default_value;
        }
        return action(option, &parsed);
    }
    }
    return false;
}

bool ConfigFile_Read(const CONFIG_IO_ARGS *const args)
{
    ASSERT(args->default_path != nullptr);
    return M_ReadFromJSON(
        args->default_path, args->enforced_path, args->action,
        args->hidden_targets);
}

bool ConfigFile_Write(const CONFIG_IO_ARGS *const args)
{
    ASSERT(args->default_path != nullptr);
    JSON_OBJECT *const root_obj = JSON_ObjectNew();
    args->action(root_obj);

    JSON_VALUE *const new_root = JSON_ValueFromObject(root_obj);
    const bool updated = JSONFile_Write(args->default_path, new_root);

    JSON_ValueFree(new_root);
    return updated;
}

void ConfigFile_LoadOptions(JSON_OBJECT *root_obj, const CONFIG_OPTION *options)
{
    const CONFIG_OPTION *opt = options;
    while (opt->target != nullptr) {
        JSON_VALUE *const value =
            JSON_ObjectGetValue(root_obj, Config_ResolveOptionName(opt->name));
        M_ProcessOptionValue(opt, value, M_SetOptionValue);
        opt++;
    }
}

void ConfigFile_DumpOptions(JSON_OBJECT *root_obj, const CONFIG_OPTION *options)
{
    const CONFIG_OPTION *opt = options;
    while (opt->target != nullptr) {
        const void *const value = Config_GetOptionValueForSave(opt);
        switch (opt->type) {
        case COT_BOOL:
            JSON_ObjectAppendBool(
                root_obj, Config_ResolveOptionName(opt->name),
                *(const bool *)value);
            break;

        case COT_INT32:
            JSON_ObjectAppendInt(
                root_obj, Config_ResolveOptionName(opt->name),
                *(const int32_t *)value);
            break;

        case COT_FLOAT:
        case COT_FLOAT_PERCENT:
            JSON_ObjectAppendDouble(
                root_obj, Config_ResolveOptionName(opt->name),
                *(const float *)value);
            break;

        case COT_DOUBLE:
            JSON_ObjectAppendDouble(
                root_obj, Config_ResolveOptionName(opt->name),
                *(const double *)value);
            break;

        case COT_ENUM:
            ConfigFile_WriteEnum(
                root_obj, Config_ResolveOptionName(opt->name),
                *(const int32_t *)value, (const char *)opt->param);
            break;

        case COT_STRING:
        case COT_DYNAMIC_ENUM: {
            const char *const string_value = *(const char *const *)value;
            if (string_value != nullptr) {
                JSON_ObjectAppendString(
                    root_obj, Config_ResolveOptionName(opt->name),
                    string_value);
            }
            break;
        }

        case COT_RGB888: {
            const RGB_888 *const color = (const RGB_888 *)value;
            char tmp[10];
            sprintf(tmp, "#%02X%02X%02X", color->r, color->g, color->b);
            JSON_ObjectAppendString(
                root_obj, Config_ResolveOptionName(opt->name), tmp);
            break;
        }
        }
        opt++;
    }
}

int ConfigFile_ReadEnum(
    JSON_OBJECT *const obj, const char *const name, const int default_value,
    const char *const enum_name)
{
    const char *value_str = JSON_ObjectGetString(obj, name, nullptr);
    if (value_str != nullptr) {
        return EnumMap_Get(enum_name, value_str, default_value);
    }
    return default_value;
}

void ConfigFile_WriteEnum(
    JSON_OBJECT *obj, const char *name, int value, const char *enum_name)
{
    JSON_ObjectAppendString(obj, name, EnumMap_ToString(enum_name, value));
}

bool ConfigFile_LoadGymTrackStats(
    JSON_OBJECT *const root_obj, const char *const key_name,
    GYM_TRACK_STATS *const stats)
{
    JSON_OBJECT *const stats_obj = JSON_ObjectGetObject(root_obj, key_name);
    if (stats_obj == nullptr) {
        return false;
    }
    JSON_ARRAY *const entries_arr = JSON_ObjectGetArray(stats_obj, "entries");
    if (entries_arr != nullptr) {
        for (size_t i = 0; i < entries_arr->length && i < MAX_ASSAULT_TIMES;
             i++) {
            JSON_OBJECT *const entry_obj = JSON_ArrayGetObject(entries_arr, i);
            if (entry_obj != nullptr) {
                stats->entries[i].time = JSON_ObjectGetInt(
                    entry_obj, "time", stats->entries[i].time);
                stats->entries[i].attempt_num = JSON_ObjectGetInt(
                    entry_obj, "attempt_num", stats->entries[i].attempt_num);
            }
        }
    }
    stats->total_attempts =
        JSON_ObjectGetInt(stats_obj, "total_attempts", stats->total_attempts);
    M_NormalizeGymTrackStats(stats);
    return true;
}

bool ConfigFile_DumpGymTrackStats(
    JSON_OBJECT *const root_obj, const char *const key_name,
    const GYM_TRACK_STATS *const stats)
{
    JSON_OBJECT *const stats_obj = JSON_ObjectNew();
    JSON_ARRAY *const entries_arr = JSON_ArrayNew();
    for (int32_t i = 0; i < MAX_ASSAULT_TIMES; i++) {
        if (stats->entries[i].time == 0) {
            break;
        }
        JSON_OBJECT *const entry_obj = JSON_ObjectNew();
        JSON_ObjectAppendInt(entry_obj, "time", stats->entries[i].time);
        JSON_ObjectAppendInt(
            entry_obj, "attempt_num", stats->entries[i].attempt_num);
        JSON_ArrayAppendObject(entries_arr, entry_obj);
    }
    JSON_ObjectAppendArray(stats_obj, "entries", entries_arr);
    JSON_ObjectAppendInt(stats_obj, "total_attempts", stats->total_attempts);
    JSON_ObjectAppendObject(root_obj, key_name, stats_obj);
    return true;
}
