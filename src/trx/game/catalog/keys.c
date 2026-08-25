#include <trx/debug.h>
#include <trx/game/catalog/manager.h>
#include <trx/game/objects/ids.h>

#include <ctype.h>
#include <string.h>

// The key of every object that names.def gives one, indexed by OBJECT_ID
static const char *const m_ObjectKeys[O_NUMBER_OF] = {
#define X_OBJ_NAMES(...)
#define X_OBJ_NAME_DEFINE(object_id_, key_name_, names_array_)                 \
    [object_id_] = key_name_,
#define X_OBJ_ALIAS_DEFINE(target_object_id_, source_object_id_)
#include <trx/game/objects/names.def>
#undef X_OBJ_ALIAS_DEFINE
#undef X_OBJ_NAME_DEFINE
#undef X_OBJ_NAMES
};

// What the C spelling of each context puts in front of the name
static const char *const m_ContextPrefixes[CATALOG_CONTEXT_MAX] = {
    [CATALOG_OBJECTS] = "O_",     [CATALOG_MUSIC] = "MX_",
    [CATALOG_SAMPLES] = "SFX_",   [CATALOG_LARA_STATES] = "LS_",
    [CATALOG_LARA_ANIMS] = "LA_", [CATALOG_ITEM_ACTIONS] = "ITEM_ACTION_",
};

const char *Catalog_KeyForEnum(
    const CATALOG_CONTEXT context, const CATALOG_ID id,
    const char *const enum_name)
{
    if (context == CATALOG_OBJECTS && id >= 0 && id < O_NUMBER_OF
        && m_ObjectKeys[id] != nullptr) {
        return m_ObjectKeys[id];
    }

    const char *const prefix = m_ContextPrefixes[context];
    const size_t prefix_len = strlen(prefix);
    const char *name = enum_name;
    if (strncmp(name, prefix, prefix_len) == 0) {
        name += prefix_len;
    }

    // The caller copies this before the next call needs it.
    static char key[64];
    ASSERT(strlen(name) < sizeof(key));
    size_t i = 0;
    for (; name[i] != '\0' && i < sizeof(key) - 1; i++) {
        key[i] = (char)tolower((unsigned char)name[i]);
    }
    key[i] = '\0';
    return key;
}
