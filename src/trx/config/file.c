#include <trx/config/file.h>

#include <trx/config/common.h>
#include <trx/config/value.h>
#include <trx/core/colors.h>
#include <trx/core/filesystem.h>
#include <trx/core/json/util/file.h>
#include <trx/core/json/util/value.h>
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
    // Every type here is addressed the same way its member is, and a string
    // value arrives as a char **, which is what Value_CopyPtr expects.
    Value_CopyPtr(option->type, (void *)option->target, value);
    return true;
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

    const bool is_text =
        option->type == TVT_STRING || option->type == TVT_DYNAMIC_ENUM;

    TRX_VALUE parsed;
    if (!JSONValue_ReadFrom(value, option->type, option->param, &parsed)) {
        // Nothing usable at this key; fall back to the option's default. A
        // string default is stored inline rather than behind a pointer.
        parsed.type = option->type;
        if (is_text) {
            parsed.as_str = (const char *)option->default_value;
        } else {
            Value_ReadPtr(option->type, option->default_value, &parsed);
        }
    }

    // The action wants a raw pointer of the option's own type: a char ** for a
    // string, and the value itself for the rest, which a CONFIG_VALUE union
    // holds at the right width.
    if (is_text) {
        return action(option, &parsed.as_str);
    }
    CONFIG_VALUE raw;
    if (Value_WritePtr(option->type, &raw, &parsed) != nullptr) {
        // The key held a number the option's storage cannot represent; fall
        // back to the default rather than applying an unchecked value.
        TRX_VALUE fallback;
        Value_ReadPtr(option->type, option->default_value, &fallback);
        Value_WritePtr(option->type, &raw, &fallback);
    }
    return action(option, &raw);
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
        // A null string is left out, so a reload falls back to its default.
        const bool is_text =
            opt->type == TVT_STRING || opt->type == TVT_DYNAMIC_ENUM;
        if (is_text && *(const char *const *)value == nullptr) {
            opt++;
            continue;
        }
        TRX_VALUE parsed;
        Value_ReadPtr(opt->type, value, &parsed);
        JSONValue_Write(
            root_obj, Config_ResolveOptionName(opt->name), opt->type,
            opt->param, &parsed);
        opt++;
    }
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
