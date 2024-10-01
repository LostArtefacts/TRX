#pragma once

#include "../../../config/option.h"
#include "../common.h"

extern CONSOLE_COMMAND g_Console_Cmd_Config;

const CONFIG_OPTION *Console_Cmd_Config_GetOptionFromKey(const char *key);
const CONFIG_OPTION *Console_Cmd_Config_GetOptionFromTarget(const void *target);
COMMAND_RESULT Console_Cmd_Config_Helper(
    const CONFIG_OPTION *option, const char *new_value);
