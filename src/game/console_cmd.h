#pragma once

#include <stdbool.h>

typedef bool (*ConsoleCmd)(const char *input);

extern ConsoleCmd g_ConsoleCommands[];
