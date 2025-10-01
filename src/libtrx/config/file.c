#include "config/file.h"

#include "colors.h"
#include "config/common.h"
#include "debug.h"
#include "filesystem.h"
#include "game/console/history.h"
#include "json_file.h"
#include "log.h"
#include "memory.h"
#include "strings.h"
#include "vector.h"

#include <stdio.h>
#include <string.h>

#define M_EMPTY_ROOT "{}"
#define M_ENFORCED_KEY "enforced_config"
#define M_HIDDEN_KEY "hidden_config"

static bool M_ReadFromJSON(
    const char *const default_path, const char *const enforced_path,
    void (*load)(JSON_OBJECT *root_obj), VECTOR *const enforced_targets,
    VECTOR *const hidden_targets)
{
    bool result = false;

    JSON_VALUE *cfg_root = JSONFile_Read(default_path);
    if (cfg_root != nullptr) {
        result = true;
    } else {
        cfg_root = JSON_ValueFromObject(JSON_ObjectNew());
    }
    JSON_VALUE *const enf_root =
        enforced_path != nullptr ? JSONFile_Read(enforced_path) : nullptr;

    JSON_OBJECT *cfg_root_obj = JSON_ValueAsObject(cfg_root);
    JSON_OBJECT *enf_root_obj = JSON_ValueAsObject(enf_root);

    // Merge settings from the game flow file.
    JSON_OBJECT *const enforced_config =
        JSON_ObjectGetObject(enf_root_obj, M_ENFORCED_KEY);
    if (enforced_config != nullptr && enforced_targets != nullptr) {
        Vector_ClearRealloc(enforced_targets);
        const JSON_OBJECT_ELEMENT *elem = enforced_config->start;
        while (elem != nullptr) {
            const char *const name = elem->name->string;
            const CONFIG_OPTION *const opt = Config_GetOptionByPath(name);
            if (opt != nullptr) {
                Vector_Add(enforced_targets, &opt->target);
            }
            elem = elem->next;
        }
    }
    if (enforced_config != nullptr) {
        JSON_ObjectMerge(cfg_root_obj, enforced_config);
    }

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

    if (cfg_root) {
        JSON_ValueFree(cfg_root);
    }
    if (enf_root) {
        JSON_ValueFree(enf_root);
    }

    return result;
}

static void M_PreserveEnforcedState(
    JSON_OBJECT *const root_obj, JSON_VALUE *const old_root,
    JSON_VALUE *const enf_root)
{
    if (old_root == nullptr || enf_root == nullptr) {
        return;
    }

    JSON_OBJECT *old_root_obj = JSON_ValueAsObject(old_root);
    JSON_OBJECT *enf_root_obj = JSON_ValueAsObject(enf_root);
    JSON_OBJECT *enforced_obj =
        JSON_ObjectGetObject(enf_root_obj, M_ENFORCED_KEY);
    if (enforced_obj == nullptr) {
        return;
    }

    // Restore the original values for any enforced settings, provided they were
    // defined.
    JSON_OBJECT_ELEMENT *elem = enforced_obj->start;
    while (elem != nullptr) {
        const char *const name = elem->name->string;
        elem = elem->next;

        JSON_ObjectEvictKey(root_obj, name);
        if (!JSON_ObjectContainsKey(old_root_obj, name)) {
            continue;
        }

        JSON_VALUE *const old_value = JSON_ObjectGetValue(old_root_obj, name);
        JSON_ObjectAppend(root_obj, name, old_value);
    }
}

bool ConfigFile_Read(const CONFIG_IO_ARGS *const args)
{
    ASSERT(args->default_path != nullptr);
    return M_ReadFromJSON(
        args->default_path, args->enforced_path, args->action,
        args->enforced_targets, args->hidden_targets);
}

bool ConfigFile_Write(const CONFIG_IO_ARGS *const args)
{
    ASSERT(args->default_path != nullptr);
    JSON_VALUE *const old_root = JSONFile_Read(args->default_path);
    JSON_VALUE *const enf_root = args->enforced_path != nullptr
        ? JSONFile_Read(args->enforced_path)
        : nullptr;

    JSON_OBJECT *const root_obj = JSON_ObjectNew();
    args->action(root_obj);
    M_PreserveEnforcedState(root_obj, old_root, enf_root);

    JSON_VALUE *const new_root = JSON_ValueFromObject(root_obj);
    const bool updated = JSONFile_Write(args->default_path, new_root);

    JSON_ValueFree(new_root);
    JSON_ValueFree(old_root);
    JSON_ValueFree(enf_root);
    return updated;
}

void ConfigFile_LoadOptions(JSON_OBJECT *root_obj, const CONFIG_OPTION *options)
{
    const CONFIG_OPTION *opt = options;
    while (opt->target != nullptr) {
        switch (opt->type) {
        case COT_BOOL:
            *(bool *)opt->target = JSON_ObjectGetBool(
                root_obj, Config_ResolveOptionName(opt->name),
                *(bool *)opt->default_value);
            break;

        case COT_INVERTED_BOOL:
            *(bool *)opt->target = !JSON_ObjectGetBool(
                root_obj, Config_ResolveOptionName(opt->name),
                *(bool *)opt->default_value);
            break;

        case COT_INT32: {
            JSON_VALUE *const value = JSON_ObjectGetValue(
                root_obj, Config_ResolveOptionName(opt->name));
            bool success = false;
            if (value != nullptr && value->type == JSON_TYPE_NUMBER) {
                *(int32_t *)opt->target =
                    JSON_ValueGetInt(value, *(int32_t *)opt->default_value);
                success = true;
            } else if (value != nullptr && value->type == JSON_TYPE_STRING) {
                success = String_ParseInteger(
                    JSON_ValueGetString(value, ""), (int32_t *)opt->target);
            }
            if (!success) {
                *(int32_t *)opt->target = *(int32_t *)opt->default_value;
            }
            break;
        }

        case COT_FLOAT:
        case COT_FLOAT_PERCENT:
            *(float *)opt->target = JSON_ObjectGetDouble(
                root_obj, Config_ResolveOptionName(opt->name),
                *(float *)opt->default_value);
            break;

        case COT_DOUBLE:
            *(double *)opt->target = JSON_ObjectGetDouble(
                root_obj, Config_ResolveOptionName(opt->name),
                *(double *)opt->default_value);
            break;

        case COT_ENUM:
            *(int *)opt->target = ConfigFile_ReadEnum(
                root_obj, Config_ResolveOptionName(opt->name),
                *(int *)opt->default_value, opt->param);
            break;

        case COT_STRING: {
            const char *const val = JSON_ObjectGetString(
                root_obj, Config_ResolveOptionName(opt->name),
                (const char *)opt->default_value);
            char **const p = (char **)opt->target;
            Memory_FreePointer(p);
            *p = Memory_DupStr(val);
            break;
        }

        case COT_RGB888: {
            RGB_888 *const target = (RGB_888 *)opt->target;
            JSON_VALUE *const value = JSON_ObjectGetValue(
                root_obj, Config_ResolveOptionName(opt->name));
            bool success = false;
            if (value != nullptr && value->type == JSON_TYPE_NUMBER) {
                const uint32_t rgb_value =
                    JSON_ValueGetInt(value, JSON_INVALID_NUMBER);
                ASSERT(rgb_value != JSON_INVALID_NUMBER);
                target->r = (rgb_value >> 0) & 0xFF;
                target->g = (rgb_value >> 8) & 0xFF;
                target->b = (rgb_value >> 16) & 0xFF;
                success = true;
            } else if (value != nullptr && value->type == JSON_TYPE_STRING) {
                const char *str_value =
                    JSON_ValueGetString(value, JSON_INVALID_STRING);
                ASSERT(str_value != JSON_INVALID_STRING);
                success = String_ParseRGB888(str_value, target);
            }
            if (!success) {
                *(RGB_888 *)opt->target = *(RGB_888 *)opt->default_value;
            }
            break;
        }
        }
        opt++;
    }
}

void ConfigFile_DumpOptions(JSON_OBJECT *root_obj, const CONFIG_OPTION *options)
{
    const CONFIG_OPTION *opt = options;
    while (opt->target != nullptr) {
        switch (opt->type) {
        case COT_BOOL:
            JSON_ObjectAppendBool(
                root_obj, Config_ResolveOptionName(opt->name),
                *(bool *)opt->target);
            break;

        case COT_INVERTED_BOOL:
            JSON_ObjectAppendBool(
                root_obj, Config_ResolveOptionName(opt->name),
                !*(bool *)opt->target);
            break;

        case COT_INT32:
            JSON_ObjectAppendInt(
                root_obj, Config_ResolveOptionName(opt->name),
                *(int32_t *)opt->target);
            break;

        case COT_FLOAT:
        case COT_FLOAT_PERCENT:
            JSON_ObjectAppendDouble(
                root_obj, Config_ResolveOptionName(opt->name),
                *(float *)opt->target);
            break;

        case COT_DOUBLE:
            JSON_ObjectAppendDouble(
                root_obj, Config_ResolveOptionName(opt->name),
                *(double *)opt->target);
            break;

        case COT_ENUM:
            ConfigFile_WriteEnum(
                root_obj, Config_ResolveOptionName(opt->name),
                *(int *)opt->target, (const char *)opt->param);
            break;

        case COT_STRING:
            JSON_ObjectAppendString(
                root_obj, Config_ResolveOptionName(opt->name),
                *(char **)opt->target);
            break;

        case COT_RGB888: {
            const RGB_888 *const color = (RGB_888 *)opt->target;
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

bool ConfigFile_LoadAssaultStats(
    JSON_OBJECT *const root_obj, ASSAULT_STATS *const assault_stats)
{
    JSON_OBJECT *const stats_obj =
        JSON_ObjectGetObject(root_obj, "assault_stats");
    if (stats_obj == nullptr) {
        return false;
    }
    JSON_ARRAY *const entries_arr = JSON_ObjectGetArray(stats_obj, "entries");
    if (entries_arr != nullptr) {
        for (size_t i = 0; i < entries_arr->length && i < MAX_ASSAULT_TIMES;
             i++) {
            JSON_OBJECT *const entry_obj = JSON_ArrayGetObject(entries_arr, i);
            if (entry_obj != nullptr) {
                assault_stats->entries[i].time = JSON_ObjectGetInt(
                    entry_obj, "time", assault_stats->entries[i].time);
                assault_stats->entries[i].attempt_num = JSON_ObjectGetInt(
                    entry_obj, "attempt_num",
                    assault_stats->entries[i].attempt_num);
            }
        }
    }
    assault_stats->total_attempts = JSON_ObjectGetInt(
        stats_obj, "total_attempts", assault_stats->total_attempts);
    return true;
}

bool ConfigFile_DumpAssaultStats(
    JSON_OBJECT *const root_obj, const ASSAULT_STATS *const assault_stats)
{
    JSON_OBJECT *const stats_obj = JSON_ObjectNew();
    JSON_ARRAY *const entries_arr = JSON_ArrayNew();
    for (int i = 0; i < MAX_ASSAULT_TIMES; i++) {
        JSON_OBJECT *const entry_obj = JSON_ObjectNew();
        JSON_ObjectAppendInt(entry_obj, "time", assault_stats->entries[i].time);
        JSON_ObjectAppendInt(
            entry_obj, "attempt_num", assault_stats->entries[i].attempt_num);
        JSON_ArrayAppendObject(entries_arr, entry_obj);
    }
    JSON_ObjectAppendArray(stats_obj, "entries", entries_arr);
    JSON_ObjectAppendInt(
        stats_obj, "total_attempts", assault_stats->total_attempts);
    JSON_ObjectAppendObject(root_obj, "assault_stats", stats_obj);
    return true;
}
