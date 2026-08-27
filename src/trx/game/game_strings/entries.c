#include <trx/game/game_strings/entries.h>

#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/subsystem.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <uthash.h>

// Resolve string references that start with "$". A value such as
// "$objects/compass_item" makes "objects/compass_option" stand for
// "objects/compass_item". A lookup with no direct value uses the nearest
// parent reference and carries over the rest of the path, so
// "objects/compass_option/name" reads "objects/compass_item/name". References
// follow the value in force at lookup time, including values supplied by a
// language file or a level.
#define M_REF_MARK '$'

// Limit one lookup before treating the reference chain as a loop.
#define M_REF_DEPTH_MAX 8

// Limit the path produced by following a reference.
#define M_KEY_SIZE 256

// One-slot-per-string for stable indirection on reload.
typedef struct M_SLOT {
    const char *value;
} M_SLOT;

typedef struct {
    char *key;
    M_SLOT *slot;
    UT_hash_handle hh;
} M_STRING_ENTRY;

static M_STRING_ENTRY *m_StringTable = nullptr;

// Return the value stored at a path without following references.
static const char *M_GetRaw(const char *const key)
{
    M_STRING_ENTRY *entry;
    HASH_FIND_STR(m_StringTable, key, entry);
    return entry != nullptr ? entry->slot->value : nullptr;
}

// Report whether a value names another path.
static bool M_IsRef(const char *const value)
{
    return value != nullptr && value[0] == M_REF_MARK;
}

// Write the referenced parent path plus the remaining path to out. Return
// false if no parent has a reference or if the result does not fit.
static bool M_FollowParentRef(const char *const key, char *const out)
{
    char head[M_KEY_SIZE];
    if ((size_t)snprintf(head, sizeof(head), "%s", key) >= sizeof(head)) {
        return false;
    }
    for (char *sep = strrchr(head, '/'); sep != nullptr;
         sep = strrchr(head, '/')) {
        *sep = '\0';
        const char *const value = M_GetRaw(head);
        if (!M_IsRef(value)) {
            continue;
        }
        char next[M_KEY_SIZE];
        if ((size_t)snprintf(
                next, sizeof(next), "%s%s", value + 1, key + strlen(head))
            >= sizeof(next)) {
            return false;
        }
        strcpy(out, next);
        return true;
    }
    return false;
}

void GameString_Reset(void)
{
    GameString_Clear();
#include <trx/game/game_strings/entries.def>
}

void GameString_Define(const char *const key, const char *value)
{
    M_STRING_ENTRY *entry;

    HASH_FIND_STR(m_StringTable, key, entry);
    if (entry == nullptr) {
        entry = Memory_Alloc(sizeof(*entry));
        entry->key = Memory_DupStr(key);
        entry->slot = Memory_Alloc(sizeof(*entry->slot));
        entry->slot->value = nullptr;
        HASH_ADD_KEYPTR(
            hh, m_StringTable, entry->key, strlen(entry->key), entry);
    }
    Memory_Free((void *)entry->slot->value);
    entry->slot->value = Memory_DupStr(value);
}

bool GameString_IsKnown(const char *const key)
{
    M_STRING_ENTRY *entry;
    HASH_FIND_STR(m_StringTable, key, entry);
    return entry != nullptr;
}

const char *GameString_Get(const char *const key)
{
    char buf[M_KEY_SIZE];
    const char *cur_key = key;
    for (int32_t depth = 0; depth < M_REF_DEPTH_MAX; depth++) {
        const char *const value = M_GetRaw(cur_key);
        if (value != nullptr && !M_IsRef(value)) {
            return value;
        }
        if (M_IsRef(value)) {
            if ((size_t)snprintf(buf, sizeof(buf), "%s", value + 1)
                >= sizeof(buf)) {
                return nullptr;
            }
        } else if (!M_FollowParentRef(cur_key, buf)) {
            return nullptr;
        }
        cur_key = buf;
    }
    LOG_ERROR("'%s' stands on a loop of references", key);
    return nullptr;
}

const char *const *GameString_GetPtr(const char *const key)
{
    M_STRING_ENTRY *entry;
    HASH_FIND_STR(m_StringTable, key, entry);
    return entry ? &entry->slot->value : nullptr;
}

void GameString_Clear(void)
{
    M_STRING_ENTRY *entry, *tmp;

    HASH_ITER(hh, m_StringTable, entry, tmp)
    {
        HASH_DEL(m_StringTable, entry);
        Memory_Free(entry->key);
        Memory_Free((void *)entry->slot->value);
        Memory_Free(entry->slot);
        Memory_Free(entry);
    }
}

REGISTER_SUBSYSTEM(.init = GameString_Reset, .shutdown = GameString_Clear)
