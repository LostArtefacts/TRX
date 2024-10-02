#include "game/console/cmd/config.h"

#include "config/common.h"
#include "config/map.h"
#include "game/game_string.h"
#include "memory.h"
#include "strings.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static const char *M_Resolve(const char *option_name);
static bool M_SameKey(const char *key1, const char *key2);
static char *M_NormalizeKey(const char *key);

static bool M_GetCurrentValue(
    const CONFIG_OPTION *option, char *target, size_t target_size);
static bool M_SetCurrentValue(
    const CONFIG_OPTION *option, const char *new_value);

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *ctx);

static const char *M_Resolve(const char *const option_name)
{
    const char *dot = strrchr(option_name, '.');
    if (dot) {
        return dot + 1;
    }
    return option_name;
}

static bool M_SameKey(const char *key1, const char *key2)
{
    key1 = M_Resolve(key1);
    key2 = M_Resolve(key2);
    const size_t len1 = strlen(key1);
    const size_t len2 = strlen(key2);
    if (len1 != len2) {
        return false;
    }
    for (uint32_t i = 0; i < len1; i++) {
        char c1 = key1[i];
        char c2 = key2[i];
        if (c1 == '_') {
            c1 = '-';
        }
        if (c2 == '_') {
            c2 = '-';
        }
        if (c1 != c2) {
            return false;
        }
    }
    return true;
}

static char *M_NormalizeKey(const char *key)
{
    // TODO: Once we support arbitrary glyphs, this conversion should
    // no longer be necessary.
    char *result = Memory_DupStr(key);
    for (uint32_t i = 0; i < strlen(result); i++) {
        if (result[i] == '_') {
            result[i] = '-';
        }
    }
    return result;
}

static bool M_GetCurrentValue(
    const CONFIG_OPTION *const option, char *target, const size_t target_size)
{
    if (option == NULL) {
        return false;
    }

    assert(option->target != NULL);
    switch (option->type) {
    case COT_BOOL:
        snprintf(
            target, target_size, "%s",
            *(bool *)option->target ? GS(MISC_ON) : GS(MISC_OFF));
        break;
    case COT_INT32:
        snprintf(target, target_size, "%d", *(int32_t *)option->target);
        break;
    case COT_FLOAT:
        snprintf(target, target_size, "%.2f", *(float *)option->target);
        break;
    case COT_DOUBLE:
        snprintf(target, target_size, "%.2f", *(double *)option->target);
        break;
    case COT_ENUM:
        snprintf(
            target, target_size, "%s",
            EnumMap_ToString(option->param, *(int32_t *)option->target));
        break;
    }
    return true;
}

static bool M_SetCurrentValue(
    const CONFIG_OPTION *const option, const char *const new_value)
{
    if (option == NULL) {
        return CR_BAD_INVOCATION;
    }

    assert(option->target != NULL);
    switch (option->type) {
    case COT_BOOL:
        if (String_Match(new_value, "on|true|1")) {
            *(bool *)option->target = true;
            return true;
        } else if (String_Match(new_value, "off|false|0")) {
            *(bool *)option->target = false;
            return true;
        }
        break;

    case COT_INT32: {
        int32_t new_value_typed;
        if (sscanf(new_value, "%d", &new_value_typed) == 1) {
            *(int32_t *)option->target = new_value_typed;
            return true;
        }
        break;
    }

    case COT_FLOAT: {
        float new_value_typed;
        if (sscanf(new_value, "%f", &new_value_typed) == 1) {
            *(float *)option->target = new_value_typed;
            return true;
        }
        break;
    }

    case COT_DOUBLE: {
        double new_value_typed;
        if (sscanf(new_value, "%lf", &new_value_typed) == 1) {
            *(double *)option->target = new_value_typed;
            return true;
        }
        break;
    }

    case COT_ENUM: {
        const int32_t new_value_typed =
            EnumMap_Get(option->param, new_value, -1);
        if (new_value_typed != -1) {
            *(int32_t *)option->target = new_value_typed;
            return true;
        }
        break;
    }
    }

    return false;
}

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *const ctx)
{
    COMMAND_RESULT result = CR_BAD_INVOCATION;

    char *key = Memory_DupStr(ctx->args);
    char *const space = strchr(key, ' ');
    const char *new_value = NULL;
    if (space != NULL) {
        new_value = space + 1;
        space[0] = '\0'; // NULL-terminate the key
    }

    const CONFIG_OPTION *const option =
        Console_Cmd_Config_GetOptionFromKey(key);
    if (option == NULL) {
        result = CR_FAILURE;
    } else {
        result = Console_Cmd_Config_Helper(option, new_value);
    }

cleanup:
    Memory_FreePointer(&key);
    return result;
}

const CONFIG_OPTION *Console_Cmd_Config_GetOptionFromKey(const char *const key)
{
    VECTOR *source = Vector_Create(sizeof(STRING_FUZZY_SOURCE));

    for (const CONFIG_OPTION *option = Config_GetOptionMap();
         option->name != NULL; option++) {
        STRING_FUZZY_SOURCE source_item = {
            .key = (const char *)M_NormalizeKey(option->name),
            .value = (void *)option,
            .weight = 1,
        };
        Vector_Add(source, &source_item);
    }

    VECTOR *matches = String_FuzzyMatch(key, source);
    const CONFIG_OPTION *result = NULL;
    if (matches->count == 0) {
        Console_Log(GS(OSD_CONFIG_OPTION_UNKNOWN_OPTION), key);
    } else if (matches->count == 1) {
        const STRING_FUZZY_MATCH *const match = Vector_Get(matches, 0);
        result = match->value;
    } else if (matches->count == 2) {
        const STRING_FUZZY_MATCH *const match1 = Vector_Get(matches, 0);
        const STRING_FUZZY_MATCH *const match2 = Vector_Get(matches, 1);
        Console_Log(GS(OSD_AMBIGUOUS_INPUT_2), match1->key, match2->key);
    } else if (matches->count >= 3) {
        const STRING_FUZZY_MATCH *const match1 = Vector_Get(matches, 0);
        const STRING_FUZZY_MATCH *const match2 = Vector_Get(matches, 1);
        Console_Log(GS(OSD_AMBIGUOUS_INPUT_3), match1->key, match2->key);
    }

    for (int32_t i = 0; i < source->count; i++) {
        const STRING_FUZZY_SOURCE *const source_item = Vector_Get(source, i);
        Memory_Free((char *)source_item->key);
    }

    Vector_Free(matches);
    Vector_Free(source);
    return result;
}

const CONFIG_OPTION *Console_Cmd_Config_GetOptionFromTarget(
    const void *const target)
{
    for (const CONFIG_OPTION *option = Config_GetOptionMap();
         option->name != NULL; option++) {
        if (option->target == target) {
            return option;
        }
    }

    return NULL;
}

COMMAND_RESULT Console_Cmd_Config_Helper(
    const CONFIG_OPTION *const option, const char *const new_value)
{
    assert(option != NULL);

    char *normalized_name = M_NormalizeKey(option->name);

    COMMAND_RESULT result = CR_BAD_INVOCATION;
    if (new_value == NULL || String_IsEmpty(new_value)) {
        char cur_value[128];
        if (M_GetCurrentValue(option, cur_value, 128)) {
            Console_Log(GS(OSD_CONFIG_OPTION_GET), normalized_name, cur_value);
            result = CR_SUCCESS;
        } else {
            result = CR_FAILURE;
        }
        return result;
    }

    if (M_SetCurrentValue(option, new_value)) {
        Config_Write();

        char final_value[128];
        assert(M_GetCurrentValue(option, final_value, 128));
        Console_Log(GS(OSD_CONFIG_OPTION_SET), normalized_name, final_value);
        result = CR_SUCCESS;
    }

cleanup:
    Memory_FreePointer(&normalized_name);
    return result;
}

CONSOLE_COMMAND g_Console_Cmd_Config = {
    .prefix = "set",
    .proc = M_Entrypoint,
};
