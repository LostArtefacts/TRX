#pragma once

#include <stddef.h>
#include <stdint.h>

// Scrollback for the dev console.

typedef struct {
    char *text;
    double expire_at;
} UI2_CONSOLE_LOG_LINE;

typedef struct {
    size_t max_lines;
    size_t vis_lines;
    UI2_CONSOLE_LOG_LINE *logs;
    int32_t listener_id;
} UI2_CONSOLE_LOGS;

// state functions
void UI2_ConsoleLogs_Init(UI2_CONSOLE_LOGS *s);
void UI2_ConsoleLogs_Free(UI2_CONSOLE_LOGS *s);
void UI2_ConsoleLogs(UI2_CONSOLE_LOGS *s);
