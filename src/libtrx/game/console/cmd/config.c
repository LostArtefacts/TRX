#include "game/console/cmd/config.h"

#include "colors.h"
#include "config.h"
#include "debug.h"
#include "enum_map.h"
#include "game/console/registry.h"
#include "game/game_string.h"
#include "memory.h"
#include "strings.h"

#include <stdio.h>
#include <string.h>

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

static const CONFIG_OPTION *M_GetOptionFromKey(const char *const key)
{
    VECTOR *source = Vector_Create(sizeof(STRING_FUZZY_SOURCE));

    for (const CONFIG_OPTION *option = Config_GetOptionMap();
         option->name != nullptr; option++) {
        STRING_FUZZY_SOURCE source_item = {
            .key = (const char *)Console_Cmd_Config_NormalizeKey(option->name),
            .value = (void *)option,
            .weight = 1,
        };
        Vector_Add(source, &source_item);
    }

    VECTOR *matches = String_FuzzyMatch(key, source);
    const CONFIG_OPTION *result = nullptr;
    if (matches->count == 0) {
        Console_LogError(GS(OSD_CONFIG_OPTION_UNKNOWN_OPTION), key);
    } else if (matches->count == 1) {
        const STRING_FUZZY_MATCH *const match = Vector_Get(matches, 0);
        result = match->value;
    } else if (matches->count == 2) {
        const STRING_FUZZY_MATCH *const match1 = Vector_Get(matches, 0);
        const STRING_FUZZY_MATCH *const match2 = Vector_Get(matches, 1);
        Console_LogError(GS(OSD_AMBIGUOUS_INPUT_2), match1->key, match2->key);
    } else if (matches->count >= 3) {
        const STRING_FUZZY_MATCH *const match1 = Vector_Get(matches, 0);
        const STRING_FUZZY_MATCH *const match2 = Vector_Get(matches, 1);
        Console_LogError(GS(OSD_AMBIGUOUS_INPUT_3), match1->key, match2->key);
    }

    for (int32_t i = 0; i < source->count; i++) {
        const STRING_FUZZY_SOURCE *const source_item = Vector_Get(source, i);
        Memory_Free((char *)source_item->key);
    }

    Vector_Free(matches);
    Vector_Free(source);
    return result;
}

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *const ctx)
{
    COMMAND_RESULT result = CR_BAD_INVOCATION;

    char *key = Memory_DupStr(ctx->args);
    char *const space = strchr(key, ' ');
    const char *new_value = nullptr;
    if (space != nullptr) {
        new_value = space + 1;
        space[0] = '\0'; // nullptr-terminate the key
    }

    const CONFIG_OPTION *const option = M_GetOptionFromKey(key);
    if (option == nullptr) {
        result = CR_FAILURE;
    } else {
        result = Console_Cmd_Config_Helper(option, new_value);
    }

cleanup:
    Memory_FreePointer(&key);
    return result;
}

// Return a comma-delimited list of valid values for the option.
// Caller must free the result with Memory_Free*().
static char *M_GetAvailableOptions(const CONFIG_OPTION *const option)
{
    if (option == nullptr) {
        return nullptr;
    }

    switch (option->type) {
    case COT_BOOL:
        return Memory_DupStr(GS(OSD_COMMAND_BOOL));

    case COT_INVERTED_BOOL:
        return Memory_DupStr(GS(OSD_COMMAND_BOOL));

    case COT_INT32:
        return Memory_DupStr(GS(OSD_COMMAND_INTEGER));

    case COT_DOUBLE:
    case COT_FLOAT:
        return Memory_DupStr(GS(OSD_COMMAND_DECIMAL));

    case COT_FLOAT_PERCENT:
        return Memory_DupStr(GS(OSD_COMMAND_PERCENT));

    case COT_ENUM: {
        const char *enum_name = (const char *)option->param;
        VECTOR *const values = EnumMap_ListValues(enum_name);
        if (values == nullptr) {
            return nullptr;
        }
        // Join vector items into a comma-separated string
        size_t total_len = 1;
        const char *const sep = ", ";
        for (int32_t i = 0; i < values->count; i++) {
            const char *const s = *(char **)Vector_Get(values, i);
            total_len += strlen(s) + (i + 1 < values->count ? strlen(sep) : 0);
        }
        char *const result = Memory_Alloc(total_len);
        char *ptr = result;
        for (int32_t i = 0; i < values->count; i++) {
            const char *const s = *(char **)Vector_Get(values, i);
            strcat(ptr, s);
            if (i + 1 < values->count) {
                strcat(ptr, sep);
            }
        }
        Vector_Free(values);
        return result;
    }

    default:
        return nullptr;
    }
}

char *Console_Cmd_Config_NormalizeKey(const char *key)
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

const CONFIG_OPTION *Console_Cmd_Config_GetOptionFromTarget(
    const void *const target)
{
    for (const CONFIG_OPTION *option = Config_GetOptionMap();
         option->name != nullptr; option++) {
        if (option->target == target) {
            return option;
        }
    }

    return nullptr;
}

COMMAND_RESULT Console_Cmd_Config_Helper(
    const CONFIG_OPTION *const option, const char *const new_value)
{
    ASSERT(option != nullptr);

    char *normalized_name = Console_Cmd_Config_NormalizeKey(option->name);

    if (new_value == nullptr || String_IsEmpty(new_value)) {
        const char *const value_str = Config_GetOptionValueAsString(option);
        if (value_str == nullptr) {
            return CR_FAILURE;
        }
        Console_Log(GS(OSD_CONFIG_OPTION_GET), normalized_name, value_str);
        return CR_SUCCESS;
    }

    COMMAND_RESULT result;
    if ((strcmp(new_value, "-") == 0
         && Config_RestoreOptionDefault(option->target))
        || Config_SetOptionValueFromString(option, new_value)) {
        Config_Update();
        const char *const value_str = Config_GetOptionValueAsString(option);
        ASSERT(value_str != nullptr);
        Console_Log(GS(OSD_CONFIG_OPTION_SET), normalized_name, value_str);
        result = CR_SUCCESS;
    } else {
        // Report bad invocation on the provided new value
        Console_LogError(GS(OSD_COMMAND_BAD_INVOCATION), new_value);
        char *available_options = M_GetAvailableOptions(option);
        if (available_options != nullptr) {
            Console_Log(GS(OSD_COMMAND_VALID_VALUES), available_options);
            Memory_FreePointer(&available_options);
        }
        result = CR_FAILURE;
    }

cleanup:
    Memory_FreePointer(&normalized_name);
    return result;
}

REGISTER_CONSOLE_COMMAND("set", M_Entrypoint, GS_ID(CONSOLE_HELP_SET))
