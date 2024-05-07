#pragma once

#include <stdbool.h>

typedef enum {
    CR_SUCCESS,
    CR_FAILURE,
    CR_UNAVAILABLE,
    CR_BAD_INVOCATION,
} COMMAND_RESULT;

typedef struct CONSOLE_COMMAND {
    const char *prefix;
    COMMAND_RESULT (*proc)(const char *args);
} CONSOLE_COMMAND;

extern CONSOLE_COMMAND g_ConsoleCommands[];
