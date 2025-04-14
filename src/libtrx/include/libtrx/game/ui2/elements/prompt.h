#pragma once

#include "../common.h"
#include "./flash.h"

#include <stdint.h>

// A text edit widget that collects text input from the player.
// Needs to be in focus to work, otherwise is inactive.

typedef struct {
    bool is_focused;
    int32_t caret_pos;
    int32_t current_text_capacity;
    char *current_text;

    int32_t listener1;
    int32_t listener2;

    UI2_FLASH_STATE flash;
} UI2_PROMPT_STATE;

// state functions
void UI2_Prompt_Init(UI2_PROMPT_STATE *s);
void UI2_Prompt_Free(UI2_PROMPT_STATE *s);
void UI2_Prompt_Control(UI2_PROMPT_STATE *s);
void UI2_Prompt_Clear(UI2_PROMPT_STATE *s);
void UI2_Prompt_SetFocus(UI2_PROMPT_STATE *s, bool is_focused);
void UI2_Prompt_ChangeText(UI2_PROMPT_STATE *s, const char *new_text);

// draw functions
void UI2_Prompt(UI2_PROMPT_STATE *s);
