#pragma once

#include "../elements/prompt.h"
#include "./console_logs.h"

// Dev console display widget.

typedef struct {
    UI2_CONSOLE_LOGS logs;
    UI2_PROMPT_STATE prompt;

    int32_t listeners[5];
    int32_t history_idx;
} UI2_CONSOLE_STATE;

// state functions
void UI2_Console_Init(UI2_CONSOLE_STATE *s);
void UI2_Console_Free(UI2_CONSOLE_STATE *s);
void UI2_Console_Control(UI2_CONSOLE_STATE *s);

// draw functions
void UI2_Console(UI2_CONSOLE_STATE *s);
