#pragma once

#include <trx/config/option.h>
#include <trx/core/colors.h>

#include <stdbool.h>
#include <stdint.h>

// A config value held apart from the option it belongs to - a copy, rather than
// the live setting. The override stack keeps the player's value in one of these
// while a script holds a different value in the option itself.
//
// An option's type says which member is the live one, so a value is never
// meaningful without the option it came from. Every function here takes both.
typedef union {
    bool bool_value;
    int32_t int32_value;
    float float_value;
    double double_value;
    RGB_888 rgb888_value;
    char *string_value;
} CONFIG_VALUE;

// Copies a raw value - the option's target, or another value's pointer - into
// dst. A string value is duplicated, so dst owns it.
void ConfigValue_Copy(
    const CONFIG_OPTION *option, CONFIG_VALUE *dst, const void *src);

// Releases whatever the value owns. Only string-typed values own anything.
void ConfigValue_Free(const CONFIG_OPTION *option, CONFIG_VALUE *value);

// The value as a raw pointer of the option's own type, which is what the rest
// of the config layer passes around.
const void *ConfigValue_GetPtr(
    const CONFIG_OPTION *option, const CONFIG_VALUE *value);

// Writes the value into the option, making it the live setting.
void ConfigValue_Apply(const CONFIG_OPTION *option, const CONFIG_VALUE *value);
