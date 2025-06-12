#include "game/objects/names.h"

#include "debug.h"
#include "game/game_string.h"
#include "game/objects/common.h"
#include "game/objects/vars.h"
#include "memory.h"
#include "strings/fuzzy_match.h"
#include "vector.h"

#include <string.h>

typedef struct {
    GAME_OBJECT_ID target_object_id;
    GAME_OBJECT_ID source_object_id;
} M_ALIAS;

typedef struct {
    GAME_OBJECT_ID object_id;
    const char *key;
    const char **default_names;
} M_DEFAULT;

typedef struct {
    VECTOR *names;
    char *description;
} M_NAME_ENTRY;

static M_NAME_ENTRY m_NamesTable[O_NUMBER_OF] = {};
static GAME_OBJECT_ID m_AliasResolver[O_NUMBER_OF] = {};

// Compile-time default names (ignoring key aliases)
static const M_DEFAULT m_Defaults[] = {
#define X_OBJ_NAMES(...) ((const char *[]) { __VA_ARGS__, nullptr })
#define X_OBJ_NAME_DEFINE(object_id_, key_name_, names_array_)                 \
    { .object_id = object_id_,                                                 \
      .key = key_name_,                                                        \
      .default_names = names_array_ },
#define X_OBJ_ALIAS_DEFINE(target_object_id, source_object_id)
#include "game/objects/names.def"
#undef X_OBJ_ALIAS_DEFINE
#undef X_OBJ_NAME_DEFINE
#undef X_OBJ_NAMES
    { .object_id = NO_OBJECT, .key = nullptr, .default_names = nullptr },
};

// Compile-time aliases (ignoring key strings and names)
static M_ALIAS m_ObjectAliases[] = {
#define X_OBJ_NAMES(...)
#define X_OBJ_NAME_DEFINE(object_id_, key_name_, default_name)
#define X_OBJ_ALIAS_DEFINE(target_object_id_, source_object_id_)               \
    { .target_object_id = target_object_id_,                                   \
      .source_object_id = source_object_id_ },
#include "game/objects/names.def"
#undef X_OBJ_ALIAS_DEFINE
#undef X_OBJ_NAME_DEFINE
#undef X_OBJ_NAMES
    { .target_object_id = NO_OBJECT },
};

static const M_DEFAULT *M_ResolveDefault(GAME_OBJECT_ID obj_id);
static M_NAME_ENTRY *M_ResolveNameEntry(GAME_OBJECT_ID obj_id);
static void M_ClearAllNames(void);

static const M_DEFAULT *M_ResolveDefault(GAME_OBJECT_ID obj_id)
{
    obj_id = m_AliasResolver[obj_id];
    for (int32_t i = 0; m_Defaults[i].object_id != NO_OBJECT; i++) {
        if (m_Defaults[i].object_id == obj_id) {
            return &m_Defaults[i];
        }
    }
    return nullptr;
}

static M_NAME_ENTRY *M_ResolveNameEntry(const GAME_OBJECT_ID obj_id)
{
    return &m_NamesTable[m_AliasResolver[obj_id]];
}

static void M_ClearAllNames(void)
{
    for (GAME_OBJECT_ID obj_id = O_FIRST; obj_id < O_NUMBER_OF; obj_id++) {
        M_NAME_ENTRY *const entry = &m_NamesTable[obj_id];
        if (entry->names != nullptr) {
            for (int32_t i = 0; i < entry->names->count; i++) {
                char *n = *(char **)Vector_Get(entry->names, i);
                Memory_FreePointer(&n);
            }
            Vector_Free(entry->names);
            entry->names = nullptr;
        }
        Memory_FreePointer(&entry->description);
    }
}

void Object_ClearNames(const GAME_OBJECT_ID obj_id)
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
}

void Object_AddName(const GAME_OBJECT_ID obj_id, const char *const name)
{
    ASSERT(obj_id >= O_FIRST && obj_id < O_NUMBER_OF);
    ASSERT(name != nullptr);
    M_NAME_ENTRY *const entry = M_ResolveNameEntry(obj_id);
    if (entry->names == nullptr) {
        entry->names = Vector_Create(sizeof(char *));
    }
    char *const dup = Memory_DupStr(name);
    Vector_Add(entry->names, &dup);
}

void Object_SetDescription(
    const GAME_OBJECT_ID obj_id, const char *const description)
{
    M_NAME_ENTRY *const entry = M_ResolveNameEntry(obj_id);
    Memory_FreePointer(&entry->description);
    if (description != nullptr) {
        entry->description = Memory_DupStr(description);
    }
}

const char *Object_GetName(const GAME_OBJECT_ID obj_id)
{
    M_NAME_ENTRY *const entry = M_ResolveNameEntry(obj_id);
    if (entry != nullptr && entry->names != nullptr
        && entry->names->count > 0) {
        return *(char **)Vector_Get(entry->names, 0);
    }
    return nullptr;
}

const char *Object_GetDescription(GAME_OBJECT_ID obj_id)
{
    M_NAME_ENTRY *const entry = M_ResolveNameEntry(obj_id);
    return entry != nullptr ? entry->description : nullptr;
}

void Object_ResetAllNames(void)
{
    M_ClearAllNames();

    // Install compile-time aliases
    for (GAME_OBJECT_ID obj_id = O_FIRST; obj_id < O_NUMBER_OF; obj_id++) {
        m_AliasResolver[obj_id] = obj_id;
    }
    for (int32_t i = 0; m_ObjectAliases[i].target_object_id != NO_OBJECT; i++) {
        const GAME_OBJECT_ID target_object_id =
            m_ObjectAliases[i].target_object_id;
        const GAME_OBJECT_ID source_object_id =
            m_ObjectAliases[i].source_object_id;
        m_AliasResolver[target_object_id] = source_object_id;
    }

    // Now apply default names
    for (size_t i = 0; m_Defaults[i].object_id != NO_OBJECT; i++) {
        for (size_t j = 0; m_Defaults[i].default_names[j] != nullptr; j++) {
            Object_AddName(
                m_Defaults[i].object_id, m_Defaults[i].default_names[j]);
        }
    }
}

OBJECT_NAME_MATCH *Object_IdsFromName(
    const char *user_input, int32_t *out_match_count,
    bool (*filter)(GAME_OBJECT_ID))
{
    VECTOR *source = Vector_Create(sizeof(STRING_FUZZY_SOURCE));

    for (GAME_OBJECT_ID obj_id = O_FIRST; obj_id < O_NUMBER_OF; obj_id++) {
        if (filter != nullptr && !filter(obj_id)) {
            continue;
        }

        const M_NAME_ENTRY *const name_entry = M_ResolveNameEntry(obj_id);
        if (name_entry->names != nullptr) {
            for (int32_t i = 0; i < name_entry->names->count; i++) {
                const char *name = *(char **)Vector_Get(name_entry->names, i);
                if (name != nullptr) {
                    STRING_FUZZY_SOURCE source_item = {
                        .key = name,
                        .value = (void *)(intptr_t)obj_id,
                        .weight = 2,
                    };
                    Vector_Add(source, &source_item);
                }
            }
        }

        if (Object_IsType(obj_id, g_PickupObjects)) {
            STRING_FUZZY_SOURCE source_item = {
                .key = "pickup",
                .value = (void *)(intptr_t)obj_id,
                .weight = 1,
            };
            Vector_Add(source, &source_item);
        }
    }

    VECTOR *matches = String_FuzzyMatch(user_input, source);

    // Fallback: if no localized matches, fuzzy-search the compile-time English
    // defaults.
    if (matches->count == 0) {
        Vector_Free(matches);
        Vector_Clear(source);

        for (GAME_OBJECT_ID obj_id = O_FIRST; obj_id < O_NUMBER_OF; obj_id++) {
            if (filter != nullptr && !filter(obj_id)) {
                continue;
            }
            const M_DEFAULT *const def = M_ResolveDefault(obj_id);
            for (const char **name = def->default_names; *name != nullptr;
                 name++) {
                // Add primary compile-time default name if it passes the filter
                STRING_FUZZY_SOURCE s = {
                    .key = *name,
                    .value = (void *)(intptr_t)obj_id,
                    .weight = 2,
                };
                Vector_Add(source, &s);
            }
        }

        matches = String_FuzzyMatch(user_input, source);
    }

    OBJECT_NAME_MATCH *results =
        Memory_Alloc(sizeof(OBJECT_NAME_MATCH) * (matches->count + 1));
    for (int32_t i = 0; i < matches->count; i++) {
        const STRING_FUZZY_MATCH *const match = Vector_Get(matches, i);
        results[i].object_id = (GAME_OBJECT_ID)(intptr_t)match->value;
        results[i].matched_name = match->key;
    }
    results[matches->count].object_id = NO_OBJECT;
    results[matches->count].matched_name = nullptr;
    if (out_match_count != nullptr) {
        *out_match_count = matches->count;
    }

    Vector_Free(matches);
    Vector_Free(source);
    return results;
}

GAME_OBJECT_ID Object_IdFromKey(const char *const key)
{
    for (int32_t i = 0; m_Defaults[i].object_id != NO_OBJECT; i++) {
        if (strcmp(m_Defaults[i].key, key) == 0) {
            return m_Defaults[i].object_id;
        }
    }
    return NO_OBJECT;
}
