#pragma once

#include <stdbool.h>

typedef bool (*ConsoleCmdProc)(const char *args);

typedef struct CONSOLE_COMMAND {
    const char *prefix;
    bool (*proc)(const char *args);
} CONSOLE_COMMAND;

extern CONSOLE_COMMAND g_ConsoleCommands[];
