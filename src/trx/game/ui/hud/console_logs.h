#pragma once

#include <stddef.h>
#include <stdint.h>

// Scrollback for the dev console.

typedef struct {
    char *text;
    // The line as it is drawn, wrapped to the width below. Both are dropped
    // when the canvas changes width, so a line follows the window it is shown
    // in rather than the one it arrived in.
    char *wrapped;
    float wrap_width;
    double expire_at;
} UI_CONSOLE_LOG_LINE;

typedef struct {
    size_t max_lines;
    size_t vis_lines;
    UI_CONSOLE_LOG_LINE *logs;
    int32_t listeners[2];
} UI_CONSOLE_LOGS;

// state functions
void UI_ConsoleLogs_Init(UI_CONSOLE_LOGS *s);
void UI_ConsoleLogs_Free(UI_CONSOLE_LOGS *s);
void UI_ConsoleLogs(UI_CONSOLE_LOGS *s);
