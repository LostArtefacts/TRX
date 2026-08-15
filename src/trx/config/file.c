#include <trx/config/file.h>

#include <trx/config/common.h>
#include <trx/config/legacy.h>
#include <trx/config/registry.h>
#include <trx/config/section.h>
#include <trx/core/filesystem.h>
#include <trx/core/json/util/file.h>
#include <trx/core/json/util/value.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/subsystem.h>
#include <trx/debug.h>

#include <string.h>

#define M_ENFORCED_KEY "enforced_config"
#define M_HIDDEN_KEY "hidden_config"

// Both documents, kept as they were read. An option registered later asks them
// what they held for it, so they outlive the read itself.
static JSON_VALUE *m_CfgRoot = nullptr;
static JSON_VALUE *m_EnfRoot = nullptr;
static bool m_CfgFound = false;

static void M_FreeRetained(void)
{
    m_CfgFound = false;
    if (m_CfgRoot != nullptr) {
        JSON_ValueFree(m_CfgRoot);
        m_CfgRoot = nullptr;
    }
    if (m_EnfRoot != nullptr) {
        JSON_ValueFree(m_EnfRoot);
        m_EnfRoot = nullptr;
    }
}

static void M_Shutdown(void)
{
    M_FreeRetained();
}

// The value at a key, as the option's own type. Falls back to the default where
// the key is missing, holds something else, or holds a number the option's
// storage cannot represent.
static void M_ReadValue(
    const CONFIG_OPTION *const option, const JSON_VALUE *const json,
    TRX_VALUE *const out)
{
    if (!Result_Absorb(JSONValue_ReadFrom(
            json, option->value.type, Config_Option_GetEnumKey(option), out))) {
        *out = option->default_value;
        return;
    }
    if (Value_CheckRange(option->value.type, out) != nullptr) {
        *out = option->default_value;
    }
}

static bool M_IsNamed(const CONFIG_OPTION *const option, const char *const name)
{
    return strcmp(option->name, name) == 0
        || strcmp(Config_ResolveOptionName(option->name), name) == 0;
}

// Everything the file already held that this build had nothing to say about.
//
// An option exists only while the game that declared it is loaded, so a key
// that belongs to no option this build has is another game's setting rather
// than one to drop. A section's key is spoken for the same way an option's is,
// whether or not the section wrote anything this time.
//
// An option the registry does have is left to the writer, whether or not it
// produced a key for it: a null string is left out so that a reload falls back
// to its default, and putting the old value back would take that away - the
// "Default" outfit would never stick. See ConfigFile_DumpOptions.
//
// A key a migration replaced goes too: it belongs to no game any more, and
// carrying it along would let the migration read it again on every launch,
// putting back the value the player changed away from - see config/legacy.h.
static void M_CarryOverUnwrittenKeys(
    JSON_OBJECT *const root_obj, const char *const path)
{
    JSON_VALUE *old_root = nullptr;
    SHOULD(JSONFile_Read(path, &old_root), "The settings keep no unknown keys");
    const JSON_OBJECT *const old_obj = JSON_ValueAsObject(old_root);
    if (old_obj != nullptr) {
        for (const JSON_OBJECT_ELEMENT *elem = old_obj->start; elem != nullptr;
             elem = elem->next) {
            const char *const name = elem->name->string;
            if (JSON_ObjectGetValue(root_obj, name) == nullptr
                && Config_FindOption(name) == nullptr
                && !Config_Section_OwnsKey(name) && !ConfigLegacy_IsKey(name)) {
                JSON_ObjectAppend(root_obj, name, JSON_ValueCopy(elem->value));
            }
        }
    }
    JSON_ValueFree(old_root);
}

void ConfigFile_Forget(void)
{
    M_FreeRetained();
}

bool ConfigFile_WasFound(void)
{
    return m_CfgFound;
}

RESULT ConfigFile_Read(
    const char *const default_path, const char *const enforced_path)
{
    ASSERT(default_path != nullptr);
    M_FreeRetained();
    m_CfgFound = FS_Exists(default_path);

    RESULT result = JSONFile_Read(default_path, &m_CfgRoot);
    if (m_CfgRoot == nullptr) {
        // The settings still have to answer for themselves when the file is
        // missing, so stand an empty document in for it.
        JSON_OBJECT *const obj = JSON_ObjectNew();
        JSON_ObjectAppendInt(obj, "config_version", -1);
        m_CfgRoot = JSON_ValueFromObject(obj);
    }
    m_EnfRoot = nullptr;
    if (enforced_path != nullptr) {
        result = Result_Merge(result, JSONFile_Read(enforced_path, &m_EnfRoot));
    }
    return result;
}

JSON_OBJECT *ConfigFile_GetRoot(void)
{
    return JSON_ValueAsObject(m_CfgRoot);
}

void ConfigFile_ApplyFileValueTo(CONFIG_OPTION *const option)
{
    ASSERT(option != nullptr);

    const JSON_OBJECT *const cfg_obj = JSON_ValueAsObject(m_CfgRoot);
    const JSON_VALUE *const json = cfg_obj != nullptr
        ? JSON_ObjectGetValue(cfg_obj, Config_ResolveOptionName(option->name))
        : nullptr;

    TRX_VALUE value = option->default_value;
    if (json != nullptr) {
        M_ReadValue(option, json, &value);
    }
    Config_Option_Write(option, &value);
}

void ConfigFile_ApplyEnforcedTo(CONFIG_OPTION *const option)
{
    ASSERT(option != nullptr);

    JSON_OBJECT *const enf_obj = JSON_ValueAsObject(m_EnfRoot);
    if (enf_obj == nullptr) {
        return;
    }

    const JSON_ARRAY *const hidden = JSON_ObjectGetArray(enf_obj, M_HIDDEN_KEY);
    if (hidden != nullptr) {
        for (size_t i = 0; i < hidden->length; i++) {
            const char *const name = JSON_ArrayGetString(hidden, i, nullptr);
            if (name == nullptr) {
                LOG_WARNING(
                    "Expected element %d in \"%s\" to be a string", i,
                    M_HIDDEN_KEY);
                continue;
            }
            if (M_IsNamed(option, name)) {
                option->flags |= CONFIG_OPTION_HIDDEN;
            }
        }
    }

    const JSON_OBJECT *const enforced =
        JSON_ObjectGetObject(enf_obj, M_ENFORCED_KEY);
    if (enforced == nullptr) {
        return;
    }
    for (const JSON_OBJECT_ELEMENT *elem = enforced->start; elem != nullptr;
         elem = elem->next) {
        if (!M_IsNamed(option, elem->name->string)) {
            continue;
        }
        // The hold goes on top of what the file carried, so the player's own
        // value survives underneath and is what the file saves.
        TRX_VALUE value;
        M_ReadValue(option, elem->value, &value);
        if (!Config_Option_PushHold(option, &value, CONFIG_HOLD_GAME_FLOW)) {
            LOG_WARNING(
                "Failed to enforce config option '%s'", elem->name->string);
        }
    }
}

RESULT ConfigFile_Write(
    const char *const default_path, void (*const action)(JSON_OBJECT *))
{
    ASSERT(default_path != nullptr);
    JSON_OBJECT *const root_obj = JSON_ObjectNew();
    action(root_obj);
    M_CarryOverUnwrittenKeys(root_obj, default_path);

    JSON_VALUE *const new_root = JSON_ValueFromObject(root_obj);
    const RESULT result = JSONFile_Write(default_path, new_root);

    JSON_ValueFree(new_root);
    return result;
}

void ConfigFile_DumpOptions(JSON_OBJECT *const root_obj)
{
    for (CONFIG_OPTION *const *opt = Config_GetOptions(); *opt != nullptr;
         opt++) {
        const CONFIG_OPTION *const option = *opt;
        // What the player chose, from underneath anything holding the option:
        // a level enforcing a setting for the evening does not get to write it
        // into the settings file.
        const TRX_VALUE *const value = Config_Option_GetBaseValue(option);
        // A null string is left out, so a reload falls back to its default.
        if ((value->type == TVT_STRING || value->type == TVT_DYNAMIC_ENUM)
            && value->as_str == nullptr) {
            continue;
        }
        JSONValue_Write(
            root_obj, Config_ResolveOptionName(option->name), value->type,
            Config_Option_GetEnumKey(option), value);
    }
}

REGISTER_SUBSYSTEM(.shutdown = M_Shutdown)
