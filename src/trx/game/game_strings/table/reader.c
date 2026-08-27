#include <trx/core/json/util/file.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/result.h>
#include <trx/core/strings.h>
#include <trx/core/utils.h>
#include <trx/core/vector.h>
#include <trx/game/game_flow.h>
#include <trx/game/game_strings/table.h>
#include <trx/game/game_strings/table/priv.h>

#include <string.h>

typedef struct {
    JSON_OBJECT *obj;
    char *prefix;
} M_STACK_ITEM;

static void M_AppendGameStringEntry(
    GS_TABLE *const out_table, size_t *const io_count,
    size_t *const io_capacity, char *const key, const char *const value)
{
    if (*io_count + 2 > *io_capacity) {
        *io_capacity *= 2;
        out_table->game_strings = Memory_Realloc(
            out_table->game_strings,
            sizeof(GS_GAME_STRING_ENTRY) * (*io_capacity));
    }
    out_table->game_strings[(*io_count)++] = (GS_GAME_STRING_ENTRY) {
        .key = key,
        .value = Memory_DupStr(value),
    };
}

static void M_LoadNestedGameStrings(
    JSON_OBJECT *const root_obj, const char *const root_key,
    GS_TABLE *const out_table, size_t *const io_count,
    size_t *const io_capacity)
{
    JSON_OBJECT *const settings_obj = JSON_ObjectGetObject(root_obj, root_key);
    if (settings_obj == nullptr) {
        return;
    }

    VECTOR *const stack = Vector_Create(sizeof(M_STACK_ITEM));
    const M_STACK_ITEM root_item = {
        .obj = settings_obj,
        .prefix = Memory_DupStr(root_key),
    };
    Vector_Add(stack, &root_item);

    while (stack->count > 0) {
        const int32_t top_idx = stack->count - 1;
        M_STACK_ITEM cur = *(M_STACK_ITEM *)Vector_Get(stack, top_idx);
        Vector_RemoveAt(stack, top_idx);

        for (JSON_OBJECT_ELEMENT *elem = cur.obj->start; elem != nullptr;
             elem = elem->next) {
            const char *const key = elem->name->string;
            if (key == JSON_INVALID_STRING) {
                LOG_WARNING("Invalid game string key");
                continue;
            }

            char *full_key = String_Format("%s/%s", cur.prefix, key);
            const char *const value =
                JSON_ValueGetString(elem->value, JSON_INVALID_STRING);
            if (value != JSON_INVALID_STRING) {
                M_AppendGameStringEntry(
                    out_table, io_count, io_capacity, full_key, value);
                continue;
            }

            JSON_OBJECT *const child = JSON_ValueAsObject(elem->value);
            if (child != nullptr) {
                const M_STACK_ITEM child_item = {
                    .obj = child,
                    .prefix = full_key,
                };
                Vector_Add(stack, &child_item);
                continue;
            }

            LOG_WARNING("Invalid game string entry '%s'", full_key);
            Memory_FreePointer(&full_key);
        }

        Memory_FreePointer(&cur.prefix);
    }

    Vector_Free(stack);
}

static void M_LoadTableFromJSON(
    JSON_OBJECT *const root_obj, GS_TABLE *const out_table)
{
    // Load localized string tables.
    const char *const nested_sections[] = {
        "general", "console", "settings", "enums", "dynamic", "objects",
    };
    size_t gs_count = 0;
    size_t gs_capacity = 0;

    for (size_t i = 0; i < ARRAY_SIZE(nested_sections); i++) {
        if (JSON_ObjectGetObject(root_obj, nested_sections[i]) == nullptr) {
            continue;
        }
        if (gs_capacity == 0) {
            gs_capacity = 64;
            out_table->game_strings =
                Memory_Alloc(sizeof(GS_GAME_STRING_ENTRY) * gs_capacity);
        }
        M_LoadNestedGameStrings(
            root_obj, nested_sections[i], out_table, &gs_count, &gs_capacity);
    }

    if (gs_capacity != 0) {
        if (gs_count + 1 > gs_capacity) {
            gs_capacity++;
            out_table->game_strings = Memory_Realloc(
                out_table->game_strings,
                sizeof(GS_GAME_STRING_ENTRY) * gs_capacity);
        }
        out_table->game_strings[gs_count] =
            (GS_GAME_STRING_ENTRY) { .key = nullptr, .value = nullptr };
    }
}

static RESULT M_LoadLevelsFromJSON(
    JSON_OBJECT *const obj, GS_FILE *const gs_file, const char *const key,
    const GF_LEVEL_TABLE_TYPE level_table_type)
{
    const GF_LEVEL_TABLE *const level_table =
        GF_GetLevelTable(level_table_type);
    GS_LEVEL_TABLE *const gs_level_table =
        &gs_file->level_tables[level_table_type];
    if (level_table->count == 0) {
        return OK;
    }

    JSON_ARRAY *const jlvl_arr = JSON_ObjectGetArray(obj, key);
    if (jlvl_arr == nullptr) {
        return OK;
    }

    FAIL_IF(
        jlvl_arr->length != (size_t)level_table->count,
        "%s: '%s' length must match with the game flow level count (got: "
        "%d, expected: %d)",
        gs_file->path, key, jlvl_arr->length, level_table->count);

    gs_level_table->count = jlvl_arr->length;
    gs_level_table->entries = Memory_Alloc(sizeof(GS_LEVEL) * jlvl_arr->length);

    JSON_ARRAY_ELEMENT *jlvl_elem = jlvl_arr->start;
    for (size_t i = 0; i < jlvl_arr->length; i++, jlvl_elem = jlvl_elem->next) {
        GS_LEVEL *const level = &gs_level_table->entries[i];

        JSON_OBJECT *const jlvl_obj = JSON_ValueAsObject(jlvl_elem->value);
        FAIL_IF(
            jlvl_obj == nullptr, "%s: '%s' elements must be dictionaries",
            gs_file->path, key);

        const char *const title =
            JSON_ObjectGetString(jlvl_obj, "title", JSON_INVALID_STRING);
        if (title != JSON_INVALID_STRING) {
            level->title = Memory_DupStr(title);
        }

        M_LoadTableFromJSON(jlvl_obj, &level->table);
    }
    return OK;
}

RESULT GS_File_CreateFromPath(
    const char *const path, const bool load_levels, GS_FILE **const out_gs_file)
{
    *out_gs_file = nullptr;

    JSON_VALUE *doc = nullptr;
    MUST(JSONFile_ReadRequired(path, &doc));

    GS_FILE *const gs_file = Memory_Alloc(sizeof(*gs_file));
    gs_file->path = Memory_DupStr(path);

    JSON_OBJECT *root_obj = JSON_ValueAsObject(doc);
    M_LoadTableFromJSON(root_obj, &gs_file->global);
    RESULT result = OK;
    if (load_levels) {
        result = Result_Merge(
            result,
            M_LoadLevelsFromJSON(root_obj, gs_file, "levels", GFLT_MAIN));
        result = Result_Merge(
            result,
            M_LoadLevelsFromJSON(root_obj, gs_file, "demos", GFLT_DEMOS));
        result = Result_Merge(
            result,
            M_LoadLevelsFromJSON(
                root_obj, gs_file, "cutscenes", GFLT_CUTSCENES));
    }

    JSON_ValueFree(doc);
    if (!IS_OK(result)) {
        GS_File_Free(gs_file);
        *out_gs_file = nullptr;
        return result;
    }
    *out_gs_file = gs_file;
    return OK;
}
