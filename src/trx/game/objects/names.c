#include <trx/game/objects/names.h>

#include <trx/core/memory.h>
#include <trx/core/subsystem.h>
#include <trx/core/vector.h>
#include <trx/debug.h>
#include <trx/game/catalog/table.h>
#include <trx/game/game_strings/entries.h>
#include <trx/game/objects/common.h>
#include <trx/game/objects/vars.h>

#include <ctype.h>
#include <string.h>

typedef struct {
    OBJECT_ID object_id;
    const char **default_names;
} M_DEFAULT;

typedef struct {
    OBJECT_ID object_id;
    const char *enum_name;
} M_KEY;

typedef struct {
    VECTOR *names;
    char *description;
    const char *slot; // stable first-name slot for this object
} M_NAME_ENTRY;

CATALOG_TABLE_DEFINE(m_NamesTable, CATALOG_OBJECTS, M_NAME_ENTRY, O_NUMBER_OF);

// Compile-time default names (ignoring key aliases)
static const M_DEFAULT m_Defaults[] = {
#define X_OBJ_NAMES(...) ((const char *[]) { __VA_ARGS__, nullptr })
#define X_OBJ_NAME_DEFINE(object_id_, names_array_)                            \
    { .object_id = object_id_, .default_names = names_array_ },
#include <trx/game/objects/names.def>
#undef X_OBJ_NAME_DEFINE
#undef X_OBJ_NAMES
    { .object_id = NO_OBJECT, .default_names = nullptr },
};

// List every object with the C spelling its key comes from.
static const M_KEY m_Keys[] = {
#define X_CATALOG_ID(enum_value_)                                              \
    { .object_id = enum_value_, .enum_name = #enum_value_ },
#include <trx/game/catalog/objects.def>
#undef X_CATALOG_ID
    { .object_id = NO_OBJECT, .enum_name = nullptr },
};

static bool M_KeyMatches(const char *const enum_name, const char *const key)
{
    const char *const name = enum_name + strlen("O_");
    size_t i = 0;
    for (; name[i] != '\0' && key[i] != '\0'; i++) {
        if ((char)tolower((unsigned char)name[i]) != key[i]) {
            return false;
        }
    }
    return name[i] == '\0' && key[i] == '\0';
}

static const M_DEFAULT *M_ResolveDefault(OBJECT_ID obj_id)
{
    for (int32_t i = 0; m_Defaults[i].object_id != NO_OBJECT; i++) {
        if (m_Defaults[i].object_id == obj_id) {
            return &m_Defaults[i];
        }
    }
    return nullptr;
}

static M_NAME_ENTRY *M_ResolveNameEntry(const OBJECT_ID obj_id)
{
    return CatalogTable_Get(&m_NamesTable, obj_id);
}

static void M_ClearAllNames(void)
{
    for (OBJECT_ID obj_id = O_FIRST; obj_id < O_NUMBER_OF; obj_id++) {
        M_NAME_ENTRY *const entry = CatalogTable_Get(&m_NamesTable, obj_id);
        if (entry->names != nullptr) {
            for (int32_t i = 0; i < entry->names->count; i++) {
                char *n = *(char **)Vector_Get(entry->names, i);
                Memory_FreePointer(&n);
            }
            Vector_Free(entry->names);
            entry->names = nullptr;
        }
        Memory_FreePointer(&entry->description);
        entry->slot = nullptr;
    }
}

static void M_Shutdown(void)
{
    M_ClearAllNames();
    CatalogTable_Free(&m_NamesTable);
}

void Object_ClearNames(const OBJECT_ID obj_id)
{
    ASSERT(obj_id >= O_FIRST && obj_id < O_NUMBER_OF);
    M_NAME_ENTRY *const entry = M_ResolveNameEntry(obj_id);
    if (entry->names != nullptr) {
        for (int32_t i = 0; i < entry->names->count; i++) {
            char *n = *(char **)Vector_Get(entry->names, i);
            Memory_FreePointer(&n);
        }
        Vector_Clear(entry->names);
    }
    entry->slot = nullptr;
}

void Object_AddName(const OBJECT_ID obj_id, const char *const name)
{
    ASSERT(obj_id >= O_FIRST && obj_id < O_NUMBER_OF);
    ASSERT(name != nullptr);
    M_NAME_ENTRY *const entry = M_ResolveNameEntry(obj_id);
    if (entry->names == nullptr) {
        entry->names = Vector_Create(sizeof(char *));
    }
    char *const dup = Memory_DupStr(name);
    Vector_Add(entry->names, &dup);
    // on first insertion, update stable slot
    if (entry->names->count == 1) {
        entry->slot = dup;
    }
}

void Object_SetDescription(
    const OBJECT_ID obj_id, const char *const description)
{
    M_NAME_ENTRY *const entry = M_ResolveNameEntry(obj_id);
    Memory_FreePointer(&entry->description);
    if (description != nullptr) {
        entry->description = Memory_DupStr(description);
    }
}

const char *Object_GetName(const OBJECT_ID obj_id)
{
    M_NAME_ENTRY *const entry = M_ResolveNameEntry(obj_id);
    return entry ? entry->slot : nullptr;
}

const char *Object_GetDescription(OBJECT_ID obj_id)
{
    M_NAME_ENTRY *const entry = M_ResolveNameEntry(obj_id);
    return entry != nullptr ? entry->description : nullptr;
}

void Object_ResetAllNames(void)
{
    M_ClearAllNames();

    // Now apply default names
    for (size_t i = 0; m_Defaults[i].object_id != NO_OBJECT; i++) {
        for (size_t j = 0; m_Defaults[i].default_names[j] != nullptr; j++) {
            Object_AddName(
                m_Defaults[i].object_id, m_Defaults[i].default_names[j]);
        }
    }
}

const VECTOR *Object_GetNames(const OBJECT_ID obj_id)
{
    return M_ResolveNameEntry(obj_id)->names;
}

const char *const *Object_GetDefaultNames(const OBJECT_ID obj_id)
{
    const M_DEFAULT *const def = M_ResolveDefault(obj_id);
    return def != nullptr ? def->default_names : nullptr;
}

OBJECT_ID Object_IdFromKey(const char *const key)
{
    for (int32_t i = 0; m_Keys[i].object_id != NO_OBJECT; i++) {
        if (M_KeyMatches(m_Keys[i].enum_name, key)) {
            return m_Keys[i].object_id;
        }
    }
    return NO_OBJECT;
}

REGISTER_SUBSYSTEM(.shutdown = M_Shutdown)
