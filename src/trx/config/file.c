#include <trx/config/file.h>

#include <trx/config/common.h>
#include <trx/config/legacy.h>
#include <trx/config/section.h>
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

    JSON_ValueFree(cfg_root);
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

// Everything the file already held that this build had nothing to say about.
//
// An option is in the map only while the game that declared it is loaded, so a
// key that belongs to no option this build has is another game's setting rather
// than one to drop. A section's key is spoken for the same way an option's is,
// whether or not the section wrote anything this time.
//
// An option the map does have is left to the writer, whether or not it produced
// a key for it: a null string is left out so that a reload falls back to its
// default, and putting the old value back would take that away - the "Default"
// outfit would never stick. See ConfigFile_DumpOptions.
//
// A key a migration replaced goes too: it belongs to no game any more, and
// carrying it along would let the migration read it again on every launch,
// putting back the value the player changed away from - see config/legacy.h.
static void M_CarryOverUnwrittenKeys(
    JSON_OBJECT *const root_obj, const char *const path)
{
    JSON_VALUE *const old_root = JSONFile_Read(path);
    const JSON_OBJECT *const old_obj = JSON_ValueAsObject(old_root);
    if (old_obj != nullptr) {
        for (const JSON_OBJECT_ELEMENT *elem = old_obj->start; elem != nullptr;
             elem = elem->next) {
            const char *const name = elem->name->string;
            if (JSON_ObjectGetValue(root_obj, name) == nullptr
                && Config_GetOptionByPath(name) == nullptr
                && !Config_Section_OwnsKey(name) && !ConfigLegacy_IsKey(name)) {
                JSON_ObjectAppend(root_obj, name, JSON_ValueCopy(elem->value));
            }
        }
    }
    JSON_ValueFree(old_root);
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
    M_CarryOverUnwrittenKeys(root_obj, args->default_path);

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
