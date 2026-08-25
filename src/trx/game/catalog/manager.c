#include <trx/game/catalog/manager.h>

#include <trx/core/csv.h>
#include <trx/core/filesystem.h>
#include <trx/core/memory.h>
#include <trx/core/subsystem.h>
#include <trx/core/utils.h>
#include <trx/debug.h>
#include <trx/game/items/actions/ids.h>
#include <trx/game/lara/enum.h>
#include <trx/game/music/ids.h>
#include <trx/game/objects/ids.h>
#include <trx/game/sound/ids.h>

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <uthash.h>

// Compile-time table of catalog IDs and their name strings
typedef struct {
    CATALOG_CONTEXT context;
    CATALOG_ID id;
    const char *name_str;
} M_ENTRY;

// Internal map from a name to the CATALOG_ID it resolves to. One identity has
// one entry for its canonical key and one for each of its aliases.
typedef struct {
    char *name_str;
    int32_t enum_value;
    UT_hash_handle hh;
} M_NAME_ENTRY;

// Internal map from game ID to CATALOG_ID
typedef struct {
    int32_t game_id;
    int32_t enum_value;
    UT_hash_handle hh;
} M_GAME_ID_ENTRY;

static const M_ENTRY m_CatalogEntryDefs[] = {
#define X_CATALOG_ID(enum_value) { CATALOG_MUSIC, enum_value, #enum_value },
#include <trx/game/catalog/music.def>
#undef X_CATALOG_ID
#define X_CATALOG_ID(enum_value) { CATALOG_OBJECTS, enum_value, #enum_value },
#include <trx/game/catalog/objects.def>
#undef X_CATALOG_ID
#define X_CATALOG_ID(enum_value) { CATALOG_SAMPLES, enum_value, #enum_value },
#include <trx/game/catalog/samples.def>
#undef X_CATALOG_ID
#define X_CATALOG_ID(enum_value)                                               \
    { CATALOG_LARA_STATES, enum_value, #enum_value },
#include <trx/game/catalog/lara_states.def>
#undef X_CATALOG_ID
#define X_CATALOG_ID(enum_value)                                               \
    { CATALOG_LARA_ANIMS, enum_value, #enum_value },
#include <trx/game/catalog/lara_anims.def>
#undef X_CATALOG_ID
#define X_CATALOG_ID(enum_value)                                               \
    { CATALOG_ITEM_ACTIONS, enum_value, #enum_value },
#include <trx/game/catalog/item_actions.def>
#undef X_CATALOG_ID
};

// Number of catalog entries
static const size_t m_CatalogEntryCount = ARRAY_SIZE(m_CatalogEntryDefs);

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

static M_NAME_ENTRY *m_Name2EnumMap[CATALOG_CONTEXT_MAX] = { nullptr };

static M_GAME_ID_ENTRY *m_GameID2EnumMap[CATALOG_CONTEXT_MAX] = { nullptr };

// The identities of each context. A resolve reads m_GameIDs by ID, so this is
// a flat array rather than anything with a lookup in it.
static char **m_Keys[CATALOG_CONTEXT_MAX] = { nullptr };
static int32_t *m_GameIDs[CATALOG_CONTEXT_MAX] = { nullptr };
static int32_t m_Counts[CATALOG_CONTEXT_MAX] = {};

// How many identities of each context the exe has a constant for
static int32_t m_BuiltInCounts[CATALOG_CONTEXT_MAX] = {};

// Helper: clear game_id->enum map
static void M_ClearGameIDMap(M_GAME_ID_ENTRY **const map)
{
    M_GAME_ID_ENTRY *cur, *tmp;
    HASH_ITER(hh, *map, cur, tmp)
    {
        HASH_DEL(*map, cur);
        Memory_Free(cur);
    }
}

static RESULT M_AddName(
    const CATALOG_CONTEXT context, const CATALOG_ID id, const char *const name)
{
    FAIL_IF(
        Catalog_FromKey(context, name, -1) >= 0,
        "context %d already holds the name '%s'", context, name);

    M_NAME_ENTRY *const entry = Memory_Alloc(sizeof(*entry));
    entry->name_str = Memory_DupStr(name);
    entry->enum_value = id;
    HASH_ADD_KEYPTR(
        hh, m_Name2EnumMap[context], entry->name_str,
        (uint32_t)strlen(entry->name_str), entry);
    return OK;
}

// The C spelling without its prefix and in lower case.
static const char *M_DerivedKey(
    const CATALOG_CONTEXT context, const char *const enum_name)
{
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

// The name a built-in is known by: what names.def calls it where it says, and
// the C spelling without its prefix where it does not.
static const char *M_KeyForEnum(
    const CATALOG_CONTEXT context, const CATALOG_ID id,
    const char *const enum_name)
{
    if (context == CATALOG_OBJECTS && m_ObjectKeys[id] != nullptr) {
        return m_ObjectKeys[id];
    }
    return M_DerivedKey(context, enum_name);
}

// Mint the built-ins, walking each .def in file order, so that the ID of a
// built-in equals the enum constant it was declared with. This runs before
// main because the mods are scanned before the subsystems come up, and that
// scan already names objects.
__attribute__((constructor)) static void M_MintBuiltIns(void)
{
    for (size_t idx = 0; idx < m_CatalogEntryCount; idx++) {
        const CATALOG_CONTEXT ctx = m_CatalogEntryDefs[idx].context;
        const char *const enum_name = m_CatalogEntryDefs[idx].name_str;
        CATALOG_ID id;
        // Two identities that resolve to the same key is a defect in
        // names.def, and the failure names both the key and this one.
        EXIT_ON_FAIL(
            Catalog_Mint(
                ctx, M_KeyForEnum(ctx, m_CatalogEntryDefs[idx].id, enum_name),
                &id),
            "cannot name %s", enum_name);
        ASSERT(id == m_CatalogEntryDefs[idx].id);
        IGNORE(Catalog_AddAlias(ctx, id, enum_name));
    }
    for (size_t ctx = 0; ctx < CATALOG_CONTEXT_MAX; ctx++) {
        m_BuiltInCounts[ctx] = m_Counts[ctx];
    }
}

// Drop what the session minted and unbind the built-ins, so that the next mod
// starts from what the exe names.
static void M_Shutdown(void)
{
    for (size_t ctx = 0; ctx < CATALOG_CONTEXT_MAX; ctx++) {
        M_ClearGameIDMap(&m_GameID2EnumMap[ctx]);
        M_NAME_ENTRY *cur, *tmp;
        HASH_ITER(hh, m_Name2EnumMap[ctx], cur, tmp)
        {
            if (cur->enum_value < m_BuiltInCounts[ctx]) {
                continue;
            }
            HASH_DEL(m_Name2EnumMap[ctx], cur);
            Memory_FreePointer(&cur->name_str);
            Memory_Free(cur);
        }
        while (m_Counts[ctx] > m_BuiltInCounts[ctx]) {
            Memory_FreePointer(&m_Keys[ctx][--m_Counts[ctx]]);
        }
        for (int32_t id = 0; id < m_Counts[ctx]; id++) {
            m_GameIDs[ctx][id] = -1;
        }
    }
}

RESULT Catalog_Mint(
    const CATALOG_CONTEXT context, const char *const key,
    CATALOG_ID *const out_id)
{
    const CATALOG_ID id = m_Counts[context];
    MUST(M_AddName(context, id, key));

    m_Counts[context]++;
    m_Keys[context] =
        Memory_Realloc(m_Keys[context], sizeof(char *) * m_Counts[context]);
    m_GameIDs[context] =
        Memory_Realloc(m_GameIDs[context], sizeof(int32_t) * m_Counts[context]);
    m_Keys[context][id] = Memory_DupStr(key);
    m_GameIDs[context][id] = -1;

    *out_id = id;
    return OK;
}

RESULT Catalog_AddAlias(
    const CATALOG_CONTEXT context, const CATALOG_ID id, const char *const alias)
{
    FAIL_IF(
        id < 0 || id >= m_Counts[context], "context %d holds no ID %d", context,
        id);
    return M_AddName(context, id, alias);
}

const char *Catalog_GetKey(const CATALOG_CONTEXT context, const CATALOG_ID id)
{
    if (id < 0 || id >= m_Counts[context]) {
        return nullptr;
    }
    return m_Keys[context][id];
}

int32_t Catalog_GetCount(const CATALOG_CONTEXT context)
{
    return m_Counts[context];
}

RESULT Catalog_BindSlot(
    const CATALOG_CONTEXT context, const CATALOG_ID id, const int32_t game_id)
{
    FAIL_IF(
        id < 0 || id >= m_Counts[context], "context %d holds no ID %d", context,
        id);
    m_GameIDs[context][id] = game_id;
    if (game_id < 0) {
        return OK;
    }

    M_GAME_ID_ENTRY *existing = nullptr;
    HASH_FIND_INT(m_GameID2EnumMap[context], &game_id, existing);
    FAIL_IF(
        existing != nullptr, "duplicate game ID %d for context %d", game_id,
        context);

    M_GAME_ID_ENTRY *const entry = Memory_Alloc(sizeof(*entry));
    entry->game_id = game_id;
    entry->enum_value = id;
    HASH_ADD_INT(m_GameID2EnumMap[context], game_id, entry);
    return OK;
}

RESULT Catalog_Load(
    const CATALOG_CONTEXT context, const char *const csv_path,
    const bool allow_duplicates)
{
    char *file_data;
    size_t file_size;
    MUST(FS_Load(csv_path, &file_data, &file_size));

    const char *pos = file_data;
    const char *end = file_data + file_size;
    char line[512];
    while (pos < end) {
        size_t len = 0;
        while (pos < end && *pos != '\n' && len + 1 < sizeof(line)) {
            line[len++] = *pos++;
        }
        if (pos < end && *pos == '\n') {
            pos++;
        }
        line[len] = '\0';
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }
        const char *p = line;
        char id_buf[32];
        char name_buf[64];
        CSV_ParseField(&p, id_buf, sizeof(id_buf));
        CSV_ParseField(&p, name_buf, sizeof(name_buf));
        char *const id_str = CSV_Trim(id_buf);
        char *const name_str = CSV_Trim(name_buf);
        const int32_t game_id = (int32_t)strtol(id_str, nullptr, 10);

        const CATALOG_ID id = Catalog_FromKey(context, name_str, -1);
        if (id < 0) {
            continue;
        }

        RESULT result = Catalog_BindSlot(context, id, game_id);
        if (allow_duplicates) {
            IGNORE(result);
        } else {
            SHOULD(result);
        }
    }
    Memory_FreePointer(&file_data);
    return OK;
}

CATALOG_ID Catalog_FromKey(
    const CATALOG_CONTEXT context, const char *const key,
    const CATALOG_ID fallback)
{
    M_NAME_ENTRY *entry = nullptr;
    HASH_FIND_STR(m_Name2EnumMap[context], key, entry);
    return entry != nullptr ? (CATALOG_ID)entry->enum_value : fallback;
}

int32_t Catalog_ToSlot(
    const CATALOG_CONTEXT context, const CATALOG_ID id, const int32_t fallback)
{
    if (id < 0 || id >= m_Counts[context] || m_GameIDs[context][id] < 0) {
        return fallback;
    }
    return m_GameIDs[context][id];
}

CATALOG_ID Catalog_FromSlot(
    const CATALOG_CONTEXT context, const int32_t slot,
    const CATALOG_ID fallback)
{
    M_GAME_ID_ENTRY *entry = nullptr;
    HASH_FIND_INT(m_GameID2EnumMap[context], &slot, entry);
    return entry != nullptr ? (CATALOG_ID)entry->enum_value : fallback;
}

REGISTER_BASE_SUBSYSTEM(.shutdown = M_Shutdown)
