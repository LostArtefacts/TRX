#include "game/game_flow.h"
#include "game/game_string_table.h"
#include "game/game_string_table/priv.h"
#include "game/shell.h"
#include "json_file.h"
#include "log.h"
#include "memory.h"

#include <string.h>

static void M_LoadTableFromJSON(
    JSON_OBJECT *const root_obj, GS_TABLE *const out_table)
{
    // Load objects
    JSON_OBJECT *const jobjs = JSON_ObjectGetObject(root_obj, "objects");
    if (jobjs != nullptr) {
        const size_t object_count = jobjs->length;
        out_table->objects =
            Memory_Alloc(sizeof(GS_OBJECT_ENTRY) * (object_count + 1));

        JSON_OBJECT_ELEMENT *jobj_elem = jobjs->start;
        for (size_t i = 0; i < object_count; i++, jobj_elem = jobj_elem->next) {
            JSON_OBJECT *const jobj_obj = JSON_ValueAsObject(jobj_elem->value);

            const char *const key = jobj_elem->name->string;
            if (key == JSON_INVALID_STRING) {
                LOG_WARNING(
                    "Invalid game string object entry %d: missing key.", i);
                continue;
            }

            const char *const single_name =
                JSON_ObjectGetString(jobj_obj, "name", JSON_INVALID_STRING);
            JSON_ARRAY *jnames_arr = JSON_ObjectGetArray(jobj_obj, "name");
            if (jnames_arr == nullptr) {
                jnames_arr = JSON_ObjectGetArray(jobj_obj, "names");
            }

            if (single_name == JSON_INVALID_STRING
                && (jnames_arr == nullptr || jnames_arr->length == 0)) {
                LOG_WARNING(
                    "Invalid game string object entry %s: missing name.", key);
                continue;
            }

            GS_OBJECT_ENTRY *const object_entry = &out_table->objects[i];
            object_entry->key = Memory_DupStr(key);
            if (jnames_arr != nullptr) {
                object_entry->names = Memory_Alloc(
                    sizeof(const char *) * (jnames_arr->length + 1));
                JSON_ARRAY_ELEMENT *elem = jnames_arr->start;
                size_t count = 0;
                for (size_t j = 0; j < jnames_arr->length;
                     j++, elem = elem->next) {
                    const char *const name =
                        JSON_ValueGetString(elem->value, JSON_INVALID_STRING);
                    if (name != JSON_INVALID_STRING) {
                        object_entry->names[count] = Memory_DupStr(name);
                        count++;
                    }
                }
                object_entry->names[count] = nullptr;
            } else {
                object_entry->names = Memory_Alloc(sizeof(const char *) * 2);
                object_entry->names[0] = Memory_DupStr(single_name);
                object_entry->names[1] = nullptr;
            }

            const char *const description = JSON_ObjectGetString(
                jobj_obj, "description", JSON_INVALID_STRING);
            object_entry->description = description != JSON_INVALID_STRING
                ? Memory_DupStr(description)
                : nullptr;
        }
    }

    // Load game_strings
    JSON_OBJECT *const jgs_obj = JSON_ObjectGetObject(root_obj, "game_strings");
    if (jgs_obj != nullptr) {
        const size_t gs_count = jgs_obj->length;
        out_table->game_strings =
            Memory_Alloc(sizeof(GS_GAME_STRING_ENTRY) * (gs_count + 1));

        JSON_OBJECT_ELEMENT *jgs_elem = jgs_obj->start;
        for (size_t i = 0; i < gs_count; i++, jgs_elem = jgs_elem->next) {
            JSON_OBJECT *const jgs_obj = JSON_ValueAsObject(jgs_elem->value);

            const char *const key = jgs_elem->name->string;
            const char *const value =
                JSON_ValueGetString(jgs_elem->value, JSON_INVALID_STRING);

            if (key == JSON_INVALID_STRING) {
                LOG_WARNING("Invalid game string entry %d: missing key.", i);
            } else if (value == JSON_INVALID_STRING) {
                LOG_WARNING("Invalid game string entry %d: missing value.", i);
            } else {
                GS_GAME_STRING_ENTRY *const gs_entry =
                    &out_table->game_strings[i];
                gs_entry->key = Memory_DupStr(key);
                gs_entry->value = Memory_DupStr(value);
            }
        }
    }
}

static void M_LoadLevelsFromJSON(
    JSON_OBJECT *const obj, GS_FILE *const gs_file, const char *const key,
    const GF_LEVEL_TABLE_TYPE level_table_type)
{
    const GF_LEVEL_TABLE *const level_table =
        GF_GetLevelTable(level_table_type);
    GS_LEVEL_TABLE *const gs_level_table =
        &gs_file->level_tables[level_table_type];
    if (level_table->count == 0) {
        return;
    }

    JSON_ARRAY *const jlvl_arr = JSON_ObjectGetArray(obj, key);
    if (jlvl_arr == nullptr) {
        return;
    }

    if (jlvl_arr->length != (size_t)level_table->count) {
        Shell_ExitSystemFmt(
            "%s: '%s' length must match with the game flow level count (got: "
            "%d, expected: %d)",
            gs_file->path, key, jlvl_arr->length, level_table->count);
    }

    gs_level_table->count = jlvl_arr->length;
    gs_level_table->entries = Memory_Alloc(sizeof(GS_LEVEL) * jlvl_arr->length);

    JSON_ARRAY_ELEMENT *jlvl_elem = jlvl_arr->start;
    for (size_t i = 0; i < jlvl_arr->length; i++, jlvl_elem = jlvl_elem->next) {
        GS_LEVEL *const level = &gs_level_table->entries[i];

        JSON_OBJECT *const jlvl_obj = JSON_ValueAsObject(jlvl_elem->value);
        if (jlvl_obj == nullptr) {
            Shell_ExitSystemFmt(
                "%s: 'levels' elements must be dictionaries", gs_file->path);
            return;
        }

        const char *const title =
            JSON_ObjectGetString(jlvl_obj, "title", JSON_INVALID_STRING);
        if (title != JSON_INVALID_STRING) {
            level->title = Memory_DupStr(title);
        }

        M_LoadTableFromJSON(jlvl_obj, &level->table);
    }
}

GS_FILE *GS_File_CreateFromPath(const char *const path, const bool load_levels)
{
    GS_FILE *const gs_file = Memory_Alloc(sizeof(*gs_file));
    gs_file->path = Memory_DupStr(path);

    JSON_VALUE *const doc =
        JSONFile_ReadEx(path, (JSON_FILE_OPTIONS) { .exit_on_error = true });
    JSON_OBJECT *root_obj = JSON_ValueAsObject(doc);
    M_LoadTableFromJSON(root_obj, &gs_file->global);
    if (load_levels) {
        M_LoadLevelsFromJSON(root_obj, gs_file, "levels", GFLT_MAIN);
        M_LoadLevelsFromJSON(root_obj, gs_file, "demos", GFLT_DEMOS);
        M_LoadLevelsFromJSON(root_obj, gs_file, "cutscenes", GFLT_CUTSCENES);
    }

    JSON_ValueFree(doc);
    return gs_file;
}
