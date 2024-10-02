#include "game/gameflow/reader.h"

#include "game/gameflow/gameflow_new.h"
#include "global/vars.h"

#include <libtrx/filesystem.h>
#include <libtrx/json.h>
#include <libtrx/log.h>
#include <libtrx/memory.h>

static void M_StringTableShutdown(GAMEFLOW_NEW_STRING_ENTRY *dest);
static bool M_LoadStringTable(
    JSON_OBJECT *root_obj, const char *key, GAMEFLOW_NEW_STRING_ENTRY **dest);
static bool M_LoadScriptLevels(JSON_OBJECT *obj, GAMEFLOW_NEW *gf);

static void M_StringTableShutdown(GAMEFLOW_NEW_STRING_ENTRY *const dest)
{
    if (dest == NULL) {
        return;
    }
    GAMEFLOW_NEW_STRING_ENTRY *cur = dest;
    while (cur->key != NULL) {
        Memory_FreePointer(&cur->key);
        Memory_FreePointer(&cur->value);
        cur++;
    }
    Memory_Free(dest);
}

static bool M_LoadStringTable(
    JSON_OBJECT *const root_obj, const char *const key,
    GAMEFLOW_NEW_STRING_ENTRY **dest)
{
    JSON_VALUE *const strings_value = JSON_ObjectGetValue(root_obj, key);
    if (strings_value == NULL) {
        // key is missing - rely on default strings
        return true;
    }

    JSON_OBJECT *const strings_obj = JSON_ValueAsObject(strings_value);
    if (strings_obj == NULL) {
        LOG_ERROR("'%s' must be a dictionary", key);
        return false;
    }

    *dest = Memory_Alloc(
        sizeof(GAMEFLOW_NEW_STRING_ENTRY) * (strings_obj->length + 1));

    GAMEFLOW_NEW_STRING_ENTRY *cur = *dest;
    JSON_OBJECT_ELEMENT *strings_elem = strings_obj->start;
    for (size_t i = 0; i < strings_obj->length;
         i++, strings_elem = strings_elem->next) {
        JSON_STRING *const value = JSON_ValueAsString(strings_elem->value);
        if (value == NULL) {
            LOG_ERROR("invalid string key %s", strings_elem->name->string);
            return NULL;
        }
        cur->key = Memory_DupStr(strings_elem->name->string);
        cur->value = Memory_DupStr(value->string);
        cur++;
    }

    cur->key = NULL;
    cur->value = NULL;
    return true;
}

static bool M_LoadScriptLevels(JSON_OBJECT *obj, GAMEFLOW_NEW *const gf)
{
    bool result = true;

    JSON_ARRAY *const jlvl_arr = JSON_ObjectGetArray(obj, "levels");
    if (jlvl_arr == NULL) {
        LOG_ERROR("'levels' must be a list");
        result = false;
        goto end;
    }

    int32_t level_count = jlvl_arr->length;
    if (level_count != g_GameFlow.num_levels) {
        LOG_ERROR(
            "'levels' must have exactly %d levels, as we still rely on legacy "
            "tombpc.dat",
            g_GameFlow.num_levels);
        result = false;
        goto end;
    }

    gf->level_count = level_count;
    gf->levels = Memory_Alloc(sizeof(GAMEFLOW_NEW_LEVEL) * level_count);

    JSON_ARRAY_ELEMENT *jlvl_elem = jlvl_arr->start;
    for (size_t i = 0; i < jlvl_arr->length; i++, jlvl_elem = jlvl_elem->next) {
        GAMEFLOW_NEW_LEVEL *const level = &gf->levels[i];

        JSON_OBJECT *const jlvl_obj = JSON_ValueAsObject(jlvl_elem->value);
        if (jlvl_obj == NULL) {
            LOG_ERROR("'levels' elements must be dictionaries");
            result = false;
            goto end;
        }

        result &= M_LoadStringTable(
            jlvl_obj, "object_strings", &level->object_strings);
        result &=
            M_LoadStringTable(jlvl_obj, "game_strings", &level->game_strings);
    }

end:
    return result;
}

bool GF_N_Load(const char *const path)
{
    GF_N_Shutdown();

    bool result = true;

    char *script_data = NULL;
    if (!File_Load(path, &script_data, NULL)) {
        LOG_ERROR("failed to open script file");
        result = false;
        goto end;
    }

    JSON_PARSE_RESULT parse_result;
    JSON_VALUE *root = JSON_ParseEx(
        script_data, strlen(script_data), JSON_PARSE_FLAGS_ALLOW_JSON5, NULL,
        NULL, &parse_result);
    if (root == NULL) {
        LOG_ERROR(
            "failed to parse script file: %s in line %d, char %d",
            JSON_GetErrorDescription(parse_result.error),
            parse_result.error_line_no, parse_result.error_row_no, script_data);
        result = false;
        goto end;
    }

    GAMEFLOW_NEW *const gf = &g_GameflowNew;
    JSON_OBJECT *root_obj = JSON_ValueAsObject(root);
    result &=
        M_LoadStringTable(root_obj, "object_strings", &gf->object_strings);
    result &= M_LoadStringTable(root_obj, "game_strings", &gf->game_strings);
    result &= M_LoadScriptLevels(root_obj, gf);

end:
    if (root) {
        JSON_ValueFree(root);
        root = NULL;
    }

    if (!result) {
        GF_N_Shutdown();
    }

    Memory_FreePointer(&script_data);
    return result;
}

void GF_N_Shutdown(void)
{
    GAMEFLOW_NEW *const gf = &g_GameflowNew;

    for (int32_t i = 0; i < gf->level_count; i++) {
        M_StringTableShutdown(gf->levels[i].object_strings);
        M_StringTableShutdown(gf->levels[i].game_strings);
    }

    M_StringTableShutdown(gf->object_strings);
    M_StringTableShutdown(gf->game_strings);
}
