#include "config/file.h"

#include "colors.h"
#include "debug.h"
#include "filesystem.h"
#include "game/console/history.h"
#include "log.h"
#include "memory.h"
#include "strings.h"

#include <string.h>

#define EMPTY_ROOT "{}"
#define ENFORCED_KEY "enforced_config"

static bool M_ReadFromJSON(
    const char *def_json, const char *enf_json,
    void (*load)(JSON_OBJECT *root_obj));
static void M_PreserveEnforcedState(
    JSON_OBJECT *root_obj, JSON_VALUE *old_root, JSON_VALUE *enf_root);
static char *M_WriteToJSON(
    void (*dump)(JSON_OBJECT *root_obj), const char *old_data,
    const char *enf_data);
static const char *M_ResolveOptionName(const char *option_name);

static JSON_VALUE *M_ReadRoot(const char *const cfg_data)
{
    if (cfg_data == nullptr) {
        return nullptr;
    }

    JSON_PARSE_RESULT parse_result;
    JSON_VALUE *root = JSON_ParseEx(
        cfg_data, strlen(cfg_data), JSON_PARSE_FLAGS_ALLOW_JSON5, nullptr,
        nullptr, &parse_result);
    if (root == nullptr) {
        LOG_ERROR(
            "failed to parse config file: %s in line %d, char %d",
            JSON_GetErrorDescription(parse_result.error),
            parse_result.error_line_no, parse_result.error_row_no);
    }

    return root;
}

static bool M_ReadFromJSON(
    const char *cfg_data, const char *enf_data,
    void (*load)(JSON_OBJECT *root_obj))
{
    bool result = false;

    JSON_VALUE *cfg_root =
        M_ReadRoot(cfg_data == nullptr ? EMPTY_ROOT : cfg_data);
    JSON_VALUE *enf_root =
        M_ReadRoot(enf_data == nullptr ? EMPTY_ROOT : enf_data);
    if (cfg_root != nullptr) {
        result = true;
    }

    JSON_OBJECT *cfg_root_obj = JSON_ValueAsObject(cfg_root);
    JSON_OBJECT *enf_root_obj = JSON_ValueAsObject(enf_root);

    JSON_OBJECT *enforced_config =
        JSON_ObjectGetObject(enf_root_obj, ENFORCED_KEY);
    if (enforced_config != nullptr) {
        JSON_ObjectMerge(cfg_root_obj, enforced_config);
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
        JSON_ObjectGetObject(enf_root_obj, ENFORCED_KEY);
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

static char *M_WriteToJSON(
    void (*dump)(JSON_OBJECT *root_obj), const char *const old_data,
    const char *const enf_data)
{
    JSON_OBJECT *root_obj = JSON_ObjectNew();

    dump(root_obj);

    JSON_VALUE *old_root = M_ReadRoot(old_data);
    JSON_VALUE *enf_root = M_ReadRoot(enf_data);
    M_PreserveEnforcedState(root_obj, old_root, enf_root);

    JSON_VALUE *root = JSON_ValueFromObject(root_obj);
    size_t size;
    char *data = JSON_WritePretty(root, "  ", "\n", &size);
    JSON_ValueFree(root);
    JSON_ValueFree(old_root);
    JSON_ValueFree(enf_root);

    return data;
}

static const char *M_ResolveOptionName(const char *option_name)
{
    const char *dot = strrchr(option_name, '.');
    if (dot) {
        return dot + 1;
    }
    return option_name;
}

bool ConfigFile_Read(const CONFIG_IO_ARGS *const args)
{
    char *default_data = nullptr;
    char *enforced_data = nullptr;

    ASSERT(args->default_path != nullptr);
    if (!File_Load(args->default_path, &default_data, nullptr)) {
        LOG_WARNING(
            "'%s' not loaded - default settings will apply",
            args->default_path);
    }

    if (args->enforced_path != nullptr) {
        File_Load(args->enforced_path, &enforced_data, nullptr);
    }

    bool result = M_ReadFromJSON(default_data, enforced_data, args->action);

    Memory_FreePointer(&default_data);
    Memory_FreePointer(&enforced_data);
    return result;
}

bool ConfigFile_Write(const CONFIG_IO_ARGS *const args)
{
    LOG_INFO("Saving user settings");

    char *old_data = nullptr;
    char *enforced_data = nullptr;

    ASSERT(args->default_path != nullptr);
    File_Load(args->default_path, &old_data, nullptr);

    if (args->enforced_path != nullptr) {
        File_Load(args->enforced_path, &enforced_data, nullptr);
    }

    bool updated = false;
    char *data = M_WriteToJSON(args->action, old_data, enforced_data);

    if (old_data == nullptr || strcmp(data, old_data) != 0) {
        MYFILE *const fp = File_Open(args->default_path, FILE_OPEN_WRITE);
        if (fp == nullptr) {
            LOG_ERROR("Failed to write settings!");
        } else {
            File_WriteData(fp, data, strlen(data));
            File_Close(fp);
            updated = true;
        }
    }

    Memory_FreePointer(&data);
    Memory_FreePointer(&old_data);
    Memory_FreePointer(&enforced_data);

    return updated;
}

void ConfigFile_LoadOptions(JSON_OBJECT *root_obj, const CONFIG_OPTION *options)
{
    const CONFIG_OPTION *opt = options;
    while (opt->target) {
        switch (opt->type) {
        case COT_BOOL:
            *(bool *)opt->target = JSON_ObjectGetBool(
                root_obj, M_ResolveOptionName(opt->name),
                *(bool *)opt->default_value);
            break;

        case COT_INT32: {
            JSON_VALUE *const value =
                JSON_ObjectGetValue(root_obj, M_ResolveOptionName(opt->name));
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
            *(float *)opt->target = JSON_ObjectGetDouble(
                root_obj, M_ResolveOptionName(opt->name),
                *(float *)opt->default_value);
            break;

        case COT_DOUBLE:
            *(double *)opt->target = JSON_ObjectGetDouble(
                root_obj, M_ResolveOptionName(opt->name),
                *(double *)opt->default_value);
            break;

        case COT_ENUM:
            *(int *)opt->target = ConfigFile_ReadEnum(
                root_obj, M_ResolveOptionName(opt->name),
                *(int *)opt->default_value, opt->param);
            break;

        case COT_RGB888: {
            JSON_VALUE *const value =
                JSON_ObjectGetValue(root_obj, M_ResolveOptionName(opt->name));
            bool success = false;
            if (value != nullptr && value->type == JSON_TYPE_NUMBER) {
                const uint32_t rgb_value =
                    JSON_ValueGetInt(value, JSON_INVALID_NUMBER);
                ASSERT(rgb_value != JSON_INVALID_NUMBER);
                RGB_888 *const target = (RGB_888 *)opt->target;
                target->r = (rgb_value >> 0) & 0xFF;
                target->g = (rgb_value >> 8) & 0xFF;
                target->b = (rgb_value >> 16) & 0xFF;
                success = true;
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
    while (opt->target) {
        switch (opt->type) {
        case COT_BOOL:
            JSON_ObjectAppendBool(
                root_obj, M_ResolveOptionName(opt->name), *(bool *)opt->target);
            break;

        case COT_INT32:
            JSON_ObjectAppendInt(
                root_obj, M_ResolveOptionName(opt->name),
                *(int32_t *)opt->target);
            break;

        case COT_FLOAT:
            JSON_ObjectAppendDouble(
                root_obj, M_ResolveOptionName(opt->name),
                *(float *)opt->target);
            break;

        case COT_DOUBLE:
            JSON_ObjectAppendDouble(
                root_obj, M_ResolveOptionName(opt->name),
                *(double *)opt->target);
            break;

        case COT_ENUM:
            ConfigFile_WriteEnum(
                root_obj, M_ResolveOptionName(opt->name), *(int *)opt->target,
                (const char *)opt->param);
            break;

        case COT_RGB888: {
            const RGB_888 *const color = (RGB_888 *)opt->target;
            JSON_ObjectAppendInt(
                root_obj, M_ResolveOptionName(opt->name),
                color->r | (color->g << 8) | (color->b << 16));
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
