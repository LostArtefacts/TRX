#include "enum_map.h"

#include "memory.h"

#include <stdio.h>
#include <uthash.h>

typedef struct {
    char *key;
    int32_t value;
    UT_hash_handle hh;
} M_ENTRY;

typedef struct {
    char *key;
    char *str_value;
    UT_hash_handle hh;
} M_INVERSE_ENTRY;

static M_ENTRY *m_Map = NULL;
static M_INVERSE_ENTRY *m_InverseMap = NULL;

void EnumMap_Define(
    const char *const enum_name, const int32_t enum_value,
    const char *const str_value)
{
    {
        const size_t key_len = strlen(enum_name) + strlen(str_value) + 2;
        char *const key = Memory_Alloc(key_len);
        snprintf(key, key_len, "%s|%s", enum_name, str_value);

        M_ENTRY *const entry = Memory_Alloc(sizeof(M_ENTRY));
        entry->key = key;
        entry->value = enum_value;
        HASH_ADD_KEYPTR(hh, m_Map, entry->key, strlen(entry->key), entry);
    }

    {
        const size_t key_len =
            snprintf(NULL, 0, "%s|%d", enum_name, enum_value) + 1;
        char *const key = Memory_Alloc(key_len);
        snprintf(key, key_len, "%s|%d", enum_name, enum_value);

        M_INVERSE_ENTRY *const entry = Memory_Alloc(sizeof(M_INVERSE_ENTRY));
        entry->key = key;
        entry->str_value = Memory_DupStr(str_value);
        HASH_ADD_KEYPTR(
            hh, m_InverseMap, entry->key, strlen(entry->key), entry);
    }
}

int32_t EnumMap_Get(
    const char *const enum_name, const char *const str_value,
    int32_t default_value)
{
    size_t key_len = strlen(enum_name) + strlen(str_value) + 2;
    char key[key_len];
    snprintf(key, key_len, "%s|%s", enum_name, str_value);

    M_ENTRY *entry;
    HASH_FIND_STR(m_Map, key, entry);
    return entry != NULL ? entry->value : default_value;
}

const char *EnumMap_ToString(
    const char *const enum_name, const int32_t enum_value)
{
    size_t key_len = snprintf(NULL, 0, "%s|%d", enum_name, enum_value) + 1;
    char key[key_len];
    snprintf(key, key_len, "%s|%d", enum_name, enum_value);

    M_INVERSE_ENTRY *entry;
    HASH_FIND_STR(m_InverseMap, key, entry);
    return entry != NULL ? entry->str_value : NULL;
}

void EnumMap_Shutdown(void)
{
    {
        M_ENTRY *current, *tmp;
        HASH_ITER(hh, m_Map, current, tmp)
        {
            HASH_DEL(m_Map, current);
            Memory_Free(current->key);
            Memory_Free(current);
        }
    }

    {
        M_INVERSE_ENTRY *current, *tmp;
        HASH_ITER(hh, m_InverseMap, current, tmp)
        {
            HASH_DEL(m_InverseMap, current);
            Memory_Free(current->str_value);
            Memory_Free(current->key);
            Memory_Free(current);
        }
    }
}
