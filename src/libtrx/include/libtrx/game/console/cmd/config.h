#pragma once

#include "../../../config/option.h"
#include "../common.h"

#include <stddef.h>

char *Console_Cmd_Config_NormalizeKey(const char *key);
bool Console_Cmd_Config_GetCurrentValue(
    const CONFIG_OPTION *option, char *target, size_t target_size);
bool Console_Cmd_Config_SetCurrentValue(
    const CONFIG_OPTION *option, const char *new_value);
const CONFIG_OPTION *Console_Cmd_Config_GetOptionFromKey(const char *key);
const CONFIG_OPTION *Console_Cmd_Config_GetOptionFromTarget(const void *target);
COMMAND_RESULT Console_Cmd_Config_Helper(
    const CONFIG_OPTION *option, const char *new_value);
