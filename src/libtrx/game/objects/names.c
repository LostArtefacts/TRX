#include "game/objects/names.h"

#include "debug.h"
#include "game/game_string.h"
#include "game/objects/common.h"
#include "game/objects/vars.h"
#include "memory.h"
#include "strings/fuzzy_match.h"

#include <string.h>

typedef struct {
    char *name;
    char *description;
} M_NAME_ENTRY;

static M_NAME_ENTRY m_NamesTable[O_NUMBER_OF] = {};
static M_NAME_ENTRY *m_NamesResolver[O_NUMBER_OF] = {};

static struct {
    GAME_OBJECT_ID object_id;
    const char *key_name;
} m_ObjectKeyNames[] = {
#define OBJ_ALIAS_DEFINE(object_id_, source_object_id_)
#define OBJ_NAME_DEFINE(object_id_, key_name_, default_name)                   \
    { .object_id = object_id_, .key_name = key_name_ },
#include "game/objects/names.def"
#undef OBJ_ALIAS_DEFINE
#undef OBJ_NAME_DEFINE
    { .object_id = NO_OBJECT },
};

// Compile-time aliases (ignoring key strings and names)
static struct {
    GAME_OBJECT_ID target_object_id;
    GAME_OBJECT_ID source_object_id;
} m_ObjectAliases[] = {
#define OBJ_ALIAS_DEFINE(target_object_id_, source_object_id_)                 \
    { .target_object_id = target_object_id_,                                   \
      .source_object_id = source_object_id_ },
#define OBJ_NAME_DEFINE(object_id_, key_name_, default_name)
#include "game/objects/names.def"
#undef OBJ_ALIAS_DEFINE
#undef OBJ_NAME_DEFINE
    { .target_object_id = NO_OBJECT },
};

static M_NAME_ENTRY *M_ResolveNameEntry(GAME_OBJECT_ID obj_id);
static void M_ClearNames(void);

static M_NAME_ENTRY *M_ResolveNameEntry(const GAME_OBJECT_ID obj_id)
{
    return m_NamesResolver[obj_id];
}

static void M_ClearNames(void)
{
    for (GAME_OBJECT_ID obj_id = O_FIRST; obj_id < O_NUMBER_OF; obj_id++) {
        M_NAME_ENTRY *const entry = &m_NamesTable[obj_id];
        Memory_FreePointer(&entry->name);
        Memory_FreePointer(&entry->description);
    }
}

void Object_SetName(const GAME_OBJECT_ID obj_id, const char *const name)
{
    ASSERT(obj_id >= O_FIRST && obj_id < O_NUMBER_OF);
    M_NAME_ENTRY *const entry = M_ResolveNameEntry(obj_id);
    Memory_FreePointer(&entry->name);
    ASSERT(name != nullptr);
    entry->name = Memory_DupStr(name);
}

void Object_SetDescription(
    const GAME_OBJECT_ID obj_id, const char *const description)
{
    M_NAME_ENTRY *const entry = M_ResolveNameEntry(obj_id);
    Memory_FreePointer(&entry->description);
    ASSERT(description != nullptr);
    entry->description = Memory_DupStr(description);
}

const char *Object_GetName(const GAME_OBJECT_ID obj_id)
{
    M_NAME_ENTRY *const entry = M_ResolveNameEntry(obj_id);
    return entry != nullptr ? entry->name : nullptr;
}

const char *Object_GetDescription(GAME_OBJECT_ID obj_id)
{
    M_NAME_ENTRY *const entry = M_ResolveNameEntry(obj_id);
    return entry != nullptr ? entry->description : nullptr;
}

void Object_ResetNames(void)
{
    M_ClearNames();

    // Install compile-time aliases
    for (GAME_OBJECT_ID obj_id = O_FIRST; obj_id < O_NUMBER_OF; obj_id++) {
        m_NamesResolver[obj_id] = &m_NamesTable[obj_id];
    }
    for (int32_t i = 0; m_ObjectAliases[i].target_object_id != NO_OBJECT; i++) {
        const GAME_OBJECT_ID target_object_id =
            m_ObjectAliases[i].target_object_id;
        const GAME_OBJECT_ID source_object_id =
            m_ObjectAliases[i].source_object_id;
        m_NamesResolver[target_object_id] = &m_NamesTable[source_object_id];
    }

    // Then set up the names
#define OBJ_ALIAS_DEFINE(target_object_id, source_object_id)
#define OBJ_NAME_DEFINE(object_id, key, name) Object_SetName(object_id, name);
#include "game/objects/names.def"
#undef OBJ_NAME_DEFINE
#undef OBJ_ALIAS_DEFINE
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
        {
            STRING_FUZZY_SOURCE source_item = {
                .key = Object_GetName(obj_id),
                .value = (void *)(intptr_t)obj_id,
                .weight = 2,
            };
            if (source_item.key != nullptr) {
                Vector_Add(source, &source_item);
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
    for (int32_t i = 0; m_ObjectKeyNames[i].object_id != NO_OBJECT; i++) {
        if (strcmp(m_ObjectKeyNames[i].key_name, key) == 0) {
            return m_ObjectKeyNames[i].object_id;
        }
    }
    return NO_OBJECT;
}
