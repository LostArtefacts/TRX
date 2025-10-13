#include "game/catalog.h"

#include "filesystem.h"
#include "game/music/ids.h"
#include "game/objects/ids.h"
#include "log.h"
#include "memory.h"

#include <ctype.h>
#include <uthash.h>

// Compile-time table of catalog IDs and their UUID strings
typedef struct {
    CATALOG_CONTEXT context;
    CATALOG_ID id;
    const char *uuid_str;
} M_ENTRY;

static const M_ENTRY m_CatalogEntryDefs[] = {
#define X_CATALOG_ID(uuid_str, enum_value)                                     \
    { CATALOG_MUSIC, enum_value, uuid_str },
#include "game/catalog_music.def"
#undef X_CATALOG_ID
#define X_CATALOG_ID(uuid_str, enum_value)                                     \
    { CATALOG_OBJECTS, enum_value, uuid_str },
#include "game/catalog_objects.def"
#undef X_CATALOG_ID
};

// Number of catalog entries
static const size_t m_CatalogEntryCount =
    sizeof(m_CatalogEntryDefs) / sizeof(m_CatalogEntryDefs[0]);

// Internal map from UUID to CATALOG_ID
typedef struct {
    UUID uuid;
    int32_t enum_value;
    UT_hash_handle hh;
} M_UUID_ENTRY;
static M_UUID_ENTRY *m_UUID2EnumMap[CATALOG_CONTEXT_MAX] = { nullptr };

// Internal map from game ID to CATALOG_ID
typedef struct {
    int32_t game_id;
    int32_t enum_value;
    UT_hash_handle hh;
} M_GAME_ID_ENTRY;
static M_GAME_ID_ENTRY *m_GameID2EnumMap[CATALOG_CONTEXT_MAX] = { nullptr };

// Parsed UUIDs and game IDs arrays (dynamically sized)
static UUID **m_CatalogUUIDs = nullptr;
static int32_t **m_CatalogGameIDs = nullptr;

// State flag
static bool m_Initialized = false;

// Helper: clear UUID->enum map
static void M_ClearUUIDMap(M_UUID_ENTRY **const map)
{
    M_UUID_ENTRY *cur, *tmp;
    HASH_ITER(hh, *map, cur, tmp)
    {
        HASH_DEL(*map, cur);
        Memory_Free(cur);
    }
}

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

// Parse a single hex digit
static bool M_HexCharValue(const char c, uint8_t *const val)
{
    if (c >= '0' && c <= '9') {
        *val = c - '0';
        return true;
    }
    if (c >= 'a' && c <= 'f') {
        *val = c - 'a' + 10;
        return true;
    }
    if (c >= 'A' && c <= 'F') {
        *val = c - 'A' + 10;
        return true;
    }
    return false;
}

// Parse UUID string into binary format
static void M_ParseUUIDString(const char *str, UUID *const uuid_out)
{
    if (str == nullptr || *str == '\0') {
        memset(uuid_out->bytes, 0, 16);
        return;
    }

    // UUID field sizes
    const int32_t field_sizes[] = { 4, 2, 2, 2, 6 };
    uint8_t *byte_ptr = uuid_out->bytes;

    for (int32_t field = 0; field < 5; field++) {
        const int32_t num_bytes = field_sizes[field];

        for (int32_t i = 0; i < num_bytes; i++) {
            while (*str == '-') {
                str++;
            }

            uint8_t hi, lo;
            if (!M_HexCharValue(*str++, &hi) || !M_HexCharValue(*str++, &lo)) {
                memset(uuid_out->bytes, 0, 16);
                return;
            }

            // For fields 0, 1, and 2, handle endianness swap
            if (field < 3) {
                byte_ptr[num_bytes - 1 - i] = (hi << 4) | lo;
            } else {
                byte_ptr[i] = (hi << 4) | lo;
            }
        }

        // Move the pointer forward for each field size
        byte_ptr += num_bytes;
    }
}

// Trim leading/trailing whitespace in-place
static char *M_StrTrim(char *s)
{
    while (*s && isspace(*s)) {
        s++;
    }
    if (*s == '\0') {
        return s;
    }
    char *end = s + strlen(s) - 1;
    while (end > s && isspace(*end)) {
        *end-- = '\0';
    }
    return s;
}

// Parse one CSV field into out buffer; advance *p past field
static void M_ParseCSVField(
    const char **const p, char *const out, const size_t out_sz)
{
    const char *src = *p;
    char *dst = out;
    const char *const end = out + out_sz - 1;
    if (*src == '"') {
        src++;
        while (*src && (*src != '"' || (src[1] == '"'))) {
            if (*src == '"' && src[1] == '"') {
                src++;
            }
            if (dst < end) {
                *dst++ = *src;
            }
            src++;
        }
        if (*src == '"') {
            src++;
        }
    } else {
        while (*src && *src != ',') {
            if (dst < end) {
                *dst++ = *src;
            }
            src++;
        }
    }
    *dst = '\0';
    if (*src == ',') {
        src++;
    }
    *p = src;
}

// Build initial maps on first load
static void M_Initialize(void)
{
    size_t count = m_CatalogEntryCount;
    m_CatalogUUIDs =
        Memory_Alloc(sizeof(*m_CatalogUUIDs) * CATALOG_CONTEXT_MAX);
    m_CatalogGameIDs =
        Memory_Alloc(sizeof(*m_CatalogGameIDs) * CATALOG_CONTEXT_MAX);
    for (size_t ctx = 0; ctx < CATALOG_CONTEXT_MAX; ctx++) {
        m_CatalogUUIDs[ctx] =
            Memory_Alloc(sizeof(*m_CatalogUUIDs[ctx]) * count);
        m_CatalogGameIDs[ctx] =
            Memory_Alloc(sizeof(*m_CatalogGameIDs[ctx]) * count);
    }
    for (size_t idx = 0; idx < count; idx++) {
        CATALOG_CONTEXT ctx = m_CatalogEntryDefs[idx].context;
        CATALOG_ID id = m_CatalogEntryDefs[idx].id;
        M_ParseUUIDString(
            m_CatalogEntryDefs[idx].uuid_str, &m_CatalogUUIDs[ctx][id]);
        m_CatalogGameIDs[ctx][id] = -1;
        M_UUID_ENTRY *const entry = Memory_Alloc(sizeof(*entry));
        entry->uuid = m_CatalogUUIDs[ctx][id];
        entry->enum_value = id;
        HASH_ADD(
            hh, m_UUID2EnumMap[ctx], uuid.bytes, sizeof(entry->uuid.bytes),
            entry);
    }
    m_Initialized = true;
}

bool Catalog_Load(const CATALOG_CONTEXT context, const char *const csv_path)
{
    if (!m_Initialized) {
        M_Initialize();
    }
    char *file_data;
    size_t file_size;
    if (!File_Load(csv_path, &file_data, &file_size)) {
        return false;
    }

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
        char uuid_buf[64];
        char id_buf[32];
        M_ParseCSVField(&p, uuid_buf, sizeof(uuid_buf));
        M_ParseCSVField(&p, id_buf, sizeof(id_buf));
        char *const uuid_str = M_StrTrim(uuid_buf);
        char *const id_str = M_StrTrim(id_buf);
        int32_t game_id = (int32_t)strtol(id_str, nullptr, 10);
        UUID uuid;
        M_ParseUUIDString(uuid_str, &uuid);

        CATALOG_ID id;
        if (!Catalog_UUIDToEnum(context, uuid, &id)) {
            continue;
        }

        m_CatalogGameIDs[context][id] = game_id;
        if (game_id >= 0) {
            M_GAME_ID_ENTRY *existing = NULL;
            HASH_FIND_INT(m_GameID2EnumMap[context], &game_id, existing);
            if (existing != NULL) {
                LOG_ERROR(
                    "Duplicate game ID %d for context %d", game_id, context);
            } else {
                M_GAME_ID_ENTRY *gentry = Memory_Alloc(sizeof(*gentry));
                gentry->game_id = game_id;
                gentry->enum_value = id;
                HASH_ADD_INT(m_GameID2EnumMap[context], game_id, gentry);
            }
        }
    }
    Memory_FreePointer(&file_data);
    return true;
}

bool Catalog_UUIDToEnum(
    const CATALOG_CONTEXT context, const UUID uuid, CATALOG_ID *const out_id)
{
    M_UUID_ENTRY *entry = NULL;
    HASH_FIND(
        hh, m_UUID2EnumMap[context], uuid.bytes, sizeof(uuid.bytes), entry);
    if (entry != nullptr) {
        *out_id = (CATALOG_ID)entry->enum_value;
        return true;
    }
    return false;
}

const UUID *Catalog_EnumToUUID(
    const CATALOG_CONTEXT context, const CATALOG_ID id)
{
    if (id < 0 || (size_t)id >= m_CatalogEntryCount) {
        return nullptr;
    }
    return &m_CatalogUUIDs[context][id];
}

bool Catalog_EnumToGameID(
    const CATALOG_CONTEXT context, const CATALOG_ID id,
    int32_t *const out_game_id)
{
    if (id < 0 || (size_t)id >= m_CatalogEntryCount) {
        return false;
    }
    const int32_t gid = m_CatalogGameIDs[context][id];
    if (gid < 0) {
        return false;
    }
    *out_game_id = gid;
    return true;
}

bool Catalog_GameIDToEnum(
    const CATALOG_CONTEXT context, const int32_t game_id,
    CATALOG_ID *const out_id)
{
    M_GAME_ID_ENTRY *entry = NULL;
    HASH_FIND_INT(m_GameID2EnumMap[context], &game_id, entry);
    if (entry != nullptr) {
        *out_id = (CATALOG_ID)entry->enum_value;
        return true;
    }
    return false;
}

void Catalog_Shutdown(void)
{
    if (!m_Initialized) {
        return;
    }
    for (size_t ctx = 0; ctx < CATALOG_CONTEXT_MAX; ctx++) {
        M_ClearGameIDMap(&m_GameID2EnumMap[ctx]);
        M_ClearUUIDMap(&m_UUID2EnumMap[ctx]);
        Memory_Free(m_CatalogUUIDs[ctx]);
        Memory_Free(m_CatalogGameIDs[ctx]);
    }
    Memory_Free(m_CatalogUUIDs);
    Memory_Free(m_CatalogGameIDs);
    m_CatalogUUIDs = nullptr;
    m_CatalogGameIDs = nullptr;
    m_Initialized = false;
}
