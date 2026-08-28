#include <trx/core/enum_map.h>

#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/debug.h>
#include <trx/game/game_strings/entries.h>

#include <ctype.h>
#include <uthash.h>

typedef struct {
    char *key;
    int32_t value;
    UT_hash_handle hh;
} M_STR_TO_ID_ENTRY;

typedef struct {
    char *key;
    char *str_value;
    UT_hash_handle hh;
} M_ID_TO_STR_ENTRY;

// The values of one enum type, in the order they were defined in. What an enum
// means is an ordering as much as a set - a menu cycles through it - and a hash
// does not keep one.
typedef struct {
    char *key;
    VECTOR *values;
    UT_hash_handle hh;
} M_TYPE_TO_IDS_ENTRY;

static M_STR_TO_ID_ENTRY *m_Str2IdMap = nullptr;
static M_ID_TO_STR_ENTRY *m_Id2StrMap = nullptr;
static M_ID_TO_STR_ENTRY *m_Id2NameMap = nullptr;
static M_ID_TO_STR_ENTRY *m_Id2LabelKeyMap = nullptr;
static M_TYPE_TO_IDS_ENTRY *m_Type2IdsMap = nullptr;

static void M_ClearStr2IdMap(M_STR_TO_ID_ENTRY **map)
{
    M_STR_TO_ID_ENTRY *current, *tmp;
    HASH_ITER(hh, *map, current, tmp)
    {
        HASH_DEL(*map, current);
        Memory_Free(current->key);
        Memory_Free(current);
    }
}

static void M_ClearId2StrMap(M_ID_TO_STR_ENTRY **map)
{
    M_ID_TO_STR_ENTRY *current, *tmp;
    HASH_ITER(hh, *map, current, tmp)
    {
        HASH_DEL(*map, current);
        Memory_Free(current->str_value);
        Memory_Free(current->key);
        Memory_Free(current);
    }
}

static void M_ClearType2IdsMap(M_TYPE_TO_IDS_ENTRY **map)
{
    M_TYPE_TO_IDS_ENTRY *current, *tmp;
    HASH_ITER(hh, *map, current, tmp)
    {
        HASH_DEL(*map, current);
        Vector_Free(current->values);
        Memory_Free(current->key);
        Memory_Free(current);
    }
}

static __attribute__((destructor)) void M_Shutdown(void)
{
    M_ClearStr2IdMap(&m_Str2IdMap);
    M_ClearId2StrMap(&m_Id2StrMap);
    M_ClearId2StrMap(&m_Id2NameMap);
    M_ClearId2StrMap(&m_Id2LabelKeyMap);
    M_ClearType2IdsMap(&m_Type2IdsMap);
}

static void M_DefineStr2Id(
    M_STR_TO_ID_ENTRY **map, const char *const enum_type_name,
    const int32_t enum_value, const char *const str_value)
{
    ASSERT(strlen(str_value) < ENUM_MAP_MAX_NAME_SIZE);
    const char *const key = String_FormatStatic(
        "%s|%s", enum_type_name, EnumMap_NormalizeName(str_value));
    M_STR_TO_ID_ENTRY *existing;
    HASH_FIND_STR(*map, key, existing);
    if (existing != nullptr) {
        if (existing->value != enum_value) {
            ASSERT_FAIL_FMT(
                "%s names both %d and %d", key, existing->value, enum_value);
        }
        return;
    }
    M_STR_TO_ID_ENTRY *const entry = Memory_Alloc(sizeof(M_STR_TO_ID_ENTRY));
    entry->key = Memory_DupStr(key);
    entry->value = enum_value;
    HASH_ADD_KEYPTR(hh, *map, entry->key, strlen(entry->key), entry);
}

static void M_DefineTypeId(
    const char *const enum_type_name, const int32_t enum_value)
{
    M_TYPE_TO_IDS_ENTRY *entry;
    HASH_FIND_STR(m_Type2IdsMap, enum_type_name, entry);
    if (entry == nullptr) {
        entry = Memory_Alloc(sizeof(M_TYPE_TO_IDS_ENTRY));
        entry->key = Memory_DupStr(enum_type_name);
        entry->values = Vector_Create(sizeof(int32_t));
        HASH_ADD_KEYPTR(
            hh, m_Type2IdsMap, entry->key, strlen(entry->key), entry);
    }
    Vector_Add(entry->values, &enum_value);
}

// Whether the mapping was new. An enum value defined twice - an alias, such as
// "jpg" and "jpeg" - is one value, and must be counted once.
static bool M_DefineId2Str(
    M_ID_TO_STR_ENTRY **map, const char *const enum_type_name,
    const int32_t enum_value, const char *const str_value)
{
    const char *const key =
        String_FormatStatic("%s|%d", enum_type_name, enum_value);
    M_ID_TO_STR_ENTRY *entry;
    HASH_FIND_STR(*map, key, entry);
    if (entry != nullptr) {
        // The inverse lookup is already defined - do not override it.
        // (This means that the first call to ENUM_MAP for a given enum value
        // also determines what serializing it back to string will pick
        // in the event there are multiple aliases).
        return false;
    }

    entry = Memory_Alloc(sizeof(M_ID_TO_STR_ENTRY));
    entry->key = Memory_DupStr(key);
    entry->str_value = Memory_DupStr(str_value);
    HASH_ADD_KEYPTR(hh, *map, entry->key, strlen(entry->key), entry);
    return true;
}

static int32_t M_Str2Id(
    M_STR_TO_ID_ENTRY *const *map, const char *const enum_type_name,
    const char *const str_value, int32_t default_value)
{
    const char *const key = String_FormatStatic(
        "%s|%s", enum_type_name, EnumMap_NormalizeName(str_value));
    M_STR_TO_ID_ENTRY *entry;
    HASH_FIND_STR(*map, key, entry);
    return entry != nullptr ? entry->value : default_value;
}

static const char *M_Id2Str(
    M_ID_TO_STR_ENTRY *const *map, const char *const enum_type_name,
    const int32_t enum_value)
{
    const char *const key =
        String_FormatStatic("%s|%d", enum_type_name, enum_value);
    M_ID_TO_STR_ENTRY *entry;
    HASH_FIND_STR(*map, key, entry);
    return entry != nullptr ? entry->str_value : nullptr;
}

static const VECTOR *M_GetTypeIds(const char *const enum_type_name)
{
    if (enum_type_name == nullptr) {
        return nullptr;
    }
    M_TYPE_TO_IDS_ENTRY *entry;
    HASH_FIND_STR(m_Type2IdsMap, enum_type_name, entry);
    return entry != nullptr ? entry->values : nullptr;
}

// Spell an enum type as it appears in a game string path.
static const char *M_LowerType(const char *const enum_type_name)
{
    static char buf[ENUM_MAP_MAX_NAME_SIZE];
    size_t i = 0;
    for (; enum_type_name[i] != '\0' && i < sizeof(buf) - 1; i++) {
        buf[i] = (char)tolower((unsigned char)enum_type_name[i]);
    }
    buf[i] = '\0';
    return buf;
}

void EnumMap_Define(
    const char *const enum_type_name, const char *const enum_name,
    const int32_t enum_value, const char *const str_value)
{
    const char *const label_key = String_FormatStatic(
        "enums/%s/%s", M_LowerType(enum_type_name),
        EnumMap_NormalizeName(str_value));
    M_DefineStr2Id(&m_Str2IdMap, enum_type_name, enum_value, str_value);
    if (M_DefineId2Str(&m_Id2StrMap, enum_type_name, enum_value, str_value)) {
        M_DefineTypeId(enum_type_name, enum_value);
    }
    M_DefineId2Str(&m_Id2NameMap, enum_type_name, enum_value, enum_name);
    M_DefineId2Str(&m_Id2LabelKeyMap, enum_type_name, enum_value, label_key);
}

const char *EnumMap_NormalizeName(const char *const name)
{
    static char buf[ENUM_MAP_MAX_NAME_SIZE];
    size_t i = 0;
    for (; name[i] != '\0' && i < sizeof(buf) - 1; i++) {
        buf[i] = (name[i] == '-' || name[i] == ':') ? '_' : name[i];
    }
    buf[i] = '\0';
    return buf;
}

void EnumMap_DefinePrefixed(
    const char *const enum_type_name, const char *const enum_name,
    const int32_t enum_value, const char *const suffix)
{
    char *const str_value = Memory_DupStr(suffix);
    for (char *c = str_value; *c != '\0'; c++) {
        *c = (char)tolower((unsigned char)*c);
    }
    EnumMap_Define(enum_type_name, enum_name, enum_value, str_value);
    Memory_Free(str_value);
}

int32_t EnumMap_Get(
    const char *const enum_type_name, const char *const str_value,
    int32_t default_value)
{
    return M_Str2Id(&m_Str2IdMap, enum_type_name, str_value, default_value);
}

const char *EnumMap_ToString(
    const char *const enum_type_name, const int32_t enum_value)
{
    return M_Id2Str(&m_Id2StrMap, enum_type_name, enum_value);
}

const char *EnumMap_GetName(
    const char *const enum_type_name, const int32_t enum_value)
{
    return M_Id2Str(&m_Id2NameMap, enum_type_name, enum_value);
}

const char *EnumMap_GetLabel(
    const char *const enum_type_name, const int32_t enum_value)
{
    const char *const key =
        M_Id2Str(&m_Id2LabelKeyMap, enum_type_name, enum_value);
    if (key == nullptr) {
        return nullptr;
    }
    return GameString_Get(key);
}

int32_t EnumMap_GetValueCount(const char *const enum_type_name)
{
    const VECTOR *const values = M_GetTypeIds(enum_type_name);
    return values != nullptr ? values->count : 0;
}

int32_t EnumMap_GetValueAt(
    const char *const enum_type_name, const int32_t index)
{
    const VECTOR *const values = M_GetTypeIds(enum_type_name);
    if (values == nullptr || index < 0 || index >= values->count) {
        return -1;
    }
    return *(const int32_t *)Vector_Get(values, index);
}

VECTOR *EnumMap_ListValues(const char *const enum_type_name)
{
    if (enum_type_name == nullptr) {
        return nullptr;
    }

    // Compare the prefix to find the matching enum values.
    const size_t prefix_len = strlen(enum_type_name) + 1;

    VECTOR *const results = Vector_Create(sizeof(char *));
    M_ID_TO_STR_ENTRY *entry;
    M_ID_TO_STR_ENTRY *tmp;
    HASH_ITER(hh, m_Id2StrMap, entry, tmp)
    {
        if (strncmp(entry->key, enum_type_name, prefix_len - 1) == 0
            && entry->key[prefix_len - 1] == '|') {
            Vector_Add(results, &entry->str_value);
        }
    }
    return results;
}
