#pragma once

#include "./types.h"

void Console_Registry_Add(CONSOLE_COMMAND cmd);
const CONSOLE_COMMAND *Console_Registry_Get(const char *cmdline);

#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)

#define REGISTER_CONSOLE_COMMAND(prefix_, proc_, help_)                        \
    __attribute__((__constructor__)) static void CONCAT(                       \
        M_Register_, __LINE__)(void)                                           \
    {                                                                          \
        Console_Registry_Add((CONSOLE_COMMAND) {                               \
            .prefix = prefix_, .proc = proc_, .help_id = help_ });             \
    }
