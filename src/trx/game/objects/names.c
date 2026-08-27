#include <trx/game/objects/names.h>

#include <trx/core/strings.h>
#include <trx/debug.h>
#include <trx/game/catalog/manager.h>
#include <trx/game/game_strings/entries.h>

#include <string.h>

typedef struct {
    OBJECT_ID object_id;
    const char *names;
} M_DEFAULT;

static const M_DEFAULT m_Defaults[] = {
#define X_OBJ_NAME_DEFINE(object_id_, names_)                                  \
    { .object_id = object_id_, .names = names_ },
#include <trx/game/objects/names.def>
#undef X_OBJ_NAME_DEFINE
    { .object_id = NO_OBJECT, .names = nullptr },
};

static const M_DEFAULT *M_GetDefault(const OBJECT_ID obj_id)
{
    for (int32_t i = 0; m_Defaults[i].object_id != NO_OBJECT; i++) {
        if (m_Defaults[i].object_id == obj_id) {
            return &m_Defaults[i];
        }
    }
    return nullptr;
}

// Return the game-string path for an object's name or description. The next
// call overwrites the result.
static const char *M_StringPath(const OBJECT_ID obj_id, const char *const what)
{
    const char *const key = Catalog_GetKey(CATALOG_OBJECTS, obj_id);
    if (key == nullptr) {
        return nullptr;
    }
    return String_FormatStatic("objects/%s/%s", key, what);
}

static const char *M_Get(const OBJECT_ID obj_id, const char *const what)
{
    const char *const path = M_StringPath(obj_id, what);
    return path != nullptr ? GameString_Get(path) : nullptr;
}

// Return the first name in a "|"-separated list.
static const char *M_HeadName(const char *const names)
{
    if (names == nullptr) {
        return nullptr;
    }
    const char *const sep = strchr(names, '|');
    if (sep == nullptr) {
        return names;
    }
    return String_FormatStatic("%.*s", (int32_t)(sep - names), names);
}

// Return the names after the first, or nullptr if the object has no aliases.
static const char *M_TailNames(const char *const names)
{
    if (names == nullptr) {
        return nullptr;
    }
    const char *const sep = strchr(names, '|');
    return sep != nullptr ? sep + 1 : nullptr;
}

const char *Object_GetName(const OBJECT_ID obj_id)
{
    return M_HeadName(M_Get(obj_id, "name"));
}

const char *Object_GetAliases(const OBJECT_ID obj_id)
{
    return M_TailNames(M_Get(obj_id, "name"));
}

const char *Object_GetDescription(const OBJECT_ID obj_id)
{
    return M_Get(obj_id, "description");
}

const char *Object_GetDefaultName(const OBJECT_ID obj_id)
{
    const M_DEFAULT *const def = M_GetDefault(obj_id);
    return def != nullptr ? M_HeadName(def->names) : nullptr;
}

const char *Object_GetDefaultAliases(const OBJECT_ID obj_id)
{
    const M_DEFAULT *const def = M_GetDefault(obj_id);
    return def != nullptr ? M_TailNames(def->names) : nullptr;
}

void Object_ResetAllNames(void)
{
    for (int32_t i = 0; m_Defaults[i].object_id != NO_OBJECT; i++) {
        GameString_Define(
            M_StringPath(m_Defaults[i].object_id, "name"), m_Defaults[i].names);
    }
}

OBJECT_ID Object_IdFromKey(const char *const key)
{
    return Catalog_FromKey(CATALOG_OBJECTS, key, NO_OBJECT);
}
