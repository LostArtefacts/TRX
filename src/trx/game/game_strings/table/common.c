#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/debug.h>
#include <trx/game/game_flow.h>
#include <trx/game/game_strings/entries.h>
#include <trx/game/game_strings/table.h>
#include <trx/game/game_strings/table/priv.h>
#include <trx/game/objects/names.h>
#include <trx/game/shell.h>

#include <string.h>

// A value may name another instead of holding words of its own. The name is
// what it stands for, after a "$": an object names another object, so that
// "objects/compass_option" holding "$objects/compass_item" takes that
// object's names and description, and a game string names another game
// string.
#define M_REF_MARK '$'
#define M_REF_OBJECTS "$objects/"

// How many references one value may go through before the chain is treated as
// a loop.
#define M_REF_DEPTH_MAX 8

typedef void (*M_LOAD_STRING_FUNC)(const char *, const char *);

static VECTOR *m_GST_Layers = nullptr;

static bool M_IsRef(const char *const value)
{
    return value != nullptr && value[0] == M_REF_MARK;
}

// The object a key names, taken from the last layer that holds it.
static const GS_OBJECT_ENTRY *M_FindObjectEntry(const char *const key)
{
    for (int32_t i = m_GST_Layers->count - 1; i >= 0; i--) {
        const GS_FILE *const gs_file = *(GS_FILE **)Vector_Get(m_GST_Layers, i);
        for (const GS_OBJECT_ENTRY *cur = gs_file->global.objects;
             cur != nullptr && cur->key != nullptr; cur++) {
            if (strcmp(cur->key, key) == 0) {
                return cur;
            }
        }
    }
    return nullptr;
}

// The game string a path names, taken from the last layer that holds it.
static const char *M_FindGameString(const char *const path)
{
    for (int32_t i = m_GST_Layers->count - 1; i >= 0; i--) {
        const GS_FILE *const gs_file = *(GS_FILE **)Vector_Get(m_GST_Layers, i);
        for (const GS_GAME_STRING_ENTRY *cur = gs_file->global.game_strings;
             cur != nullptr && cur->key != nullptr; cur++) {
            if (strcmp(cur->key, path) == 0) {
                return cur->value;
            }
        }
    }
    return nullptr;
}

// Follow the objects an object stands for and return the one that holds words
// of its own, or null where a reference names nothing or the chain runs past
// M_REF_DEPTH_MAX.
static const GS_OBJECT_ENTRY *M_ResolveObjectEntry(
    const GS_OBJECT_ENTRY *const entry, const int32_t depth)
{
    if (entry == nullptr || entry->ref == nullptr) {
        return entry;
    }
    if (depth >= M_REF_DEPTH_MAX) {
        LOG_ERROR("'%s' stands on a loop of references", entry->ref);
        return nullptr;
    }
    if (strncmp(entry->ref, M_REF_OBJECTS, strlen(M_REF_OBJECTS)) != 0) {
        LOG_ERROR("'%s' names no object", entry->ref);
        return nullptr;
    }
    const GS_OBJECT_ENTRY *const target =
        M_FindObjectEntry(entry->ref + strlen(M_REF_OBJECTS));
    if (target == nullptr) {
        LOG_ERROR("'%s' names no object", entry->ref);
        return nullptr;
    }
    return M_ResolveObjectEntry(target, depth + 1);
}

// The same for a game string.
static const char *M_ResolveValue(const char *const value, const int32_t depth)
{
    if (!M_IsRef(value)) {
        return value;
    }
    if (depth >= M_REF_DEPTH_MAX) {
        LOG_ERROR("'%s' stands on a loop of references", value);
        return nullptr;
    }
    const char *const target = M_FindGameString(value + 1);
    if (target == nullptr) {
        LOG_ERROR("'%s' names no game string", value);
        return nullptr;
    }
    return M_ResolveValue(target, depth + 1);
}

static void M_Apply(const GS_TABLE *const table)
{
    for (const GS_GAME_STRING_ENTRY *cur = table->game_strings;
         cur != nullptr && cur->key != nullptr; cur++) {
        const char *const value = M_ResolveValue(cur->value, 0);
        if (value == nullptr) {
            LOG_ERROR("Invalid game string value: %s", cur->key);
        } else {
            GameString_Define(cur->key, value);
        }
    }

    for (const GS_OBJECT_ENTRY *cur = table->objects;
         cur != nullptr && cur->key != nullptr; cur++) {
        const OBJECT_ID obj_id = Object_IdFromKey(cur->key);
        if (obj_id == NO_OBJECT) {
            LOG_ERROR("Invalid object id: %s", cur->key);
        } else {
            const GS_OBJECT_ENTRY *const entry = M_ResolveObjectEntry(cur, 0);
            if (entry == nullptr || entry->names == nullptr) {
                LOG_ERROR("Invalid object name(s): %s", cur->key);
                continue;
            }
            Object_ClearNames(obj_id);
            for (const char *const *name = entry->names; *name != nullptr;
                 name++) {
                Object_AddName(obj_id, *name);
            }
            Object_SetDescription(obj_id, entry->description);
        }
    }
}

static void M_ApplyLevelTitles(
    const GS_FILE *const gs_file, const GF_LEVEL_TABLE_TYPE level_table_type)
{
    const GF_LEVEL_TABLE *const level_table =
        GF_GetLevelTable(level_table_type);
    const GS_LEVEL_TABLE *const gs_level_table =
        &gs_file->level_tables[level_table_type];
    if (gs_level_table->count == 0) {
        return;
    }
    ASSERT(gs_level_table->count == level_table->count);
    for (int32_t i = 0; i < level_table->count; i++) {
        if (gs_level_table->entries[i].title != nullptr) {
            GF_SetLevelTitle(
                &level_table->levels[i], gs_level_table->entries[i].title);
        }
    }
}

static void M_ApplyLayer(
    const GF_LEVEL *const level, const GS_FILE *const gs_file)
{
    LOG_DEBUG("applying layer: %s", gs_file->path);
    M_Apply(&gs_file->global);

    for (int32_t i = 0; i < GFLT_NUMBER_OF; i++) {
        M_ApplyLevelTitles(gs_file, i);
    }

    if (level != nullptr) {
        const GS_LEVEL_TABLE *gs_level_table = nullptr;
        switch (level->type) {
        case GFL_TITLE:
            gs_level_table = nullptr;
            break;
        default: {
            const GF_LEVEL_TABLE_TYPE level_table_type =
                GF_GetLevelTableType(level->type);
            gs_level_table = &gs_file->level_tables[level_table_type];
        }
        }

        if (gs_level_table != nullptr && gs_level_table->count != 0) {
            ASSERT(level->num >= 0);
            ASSERT(level->num < gs_level_table->count);
            M_Apply(&gs_level_table->entries[level->num].table);
        }
    }
}

void GameStringTable_Apply(const GF_LEVEL *const level)
{
    Object_ResetAllNames();
    ASSERT(m_GST_Layers != nullptr);
    for (int32_t i = 0; i < m_GST_Layers->count; i++) {
        const GS_FILE *const gs_file = *(GS_FILE **)Vector_Get(m_GST_Layers, i);
        M_ApplyLayer(level, gs_file);
    }
}

void GameStringTable_Init(void)
{
    m_GST_Layers = Vector_Create(sizeof(GS_FILE *));
}

void GameStringTable_Shutdown(void)
{
    if (m_GST_Layers != nullptr) {
        for (int32_t i = 0; i < m_GST_Layers->count; i++) {
            GS_FILE *const gs_file = *(GS_FILE **)Vector_Get(m_GST_Layers, i);
            GS_File_Free(gs_file);
        }
        Vector_Free(m_GST_Layers);
        m_GST_Layers = nullptr;
    }
}

RESULT GameStringTable_Load(const char *const path, const bool load_levels)
{
    GS_FILE *gs_file = nullptr;
    MUST(GS_File_CreateFromPath(path, load_levels, &gs_file));
    ASSERT(m_GST_Layers != nullptr);
    Vector_Add(m_GST_Layers, &gs_file);
    return OK;
}
