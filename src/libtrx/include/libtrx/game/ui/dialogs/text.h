#pragma once

#include "../../../vector.h"
#include "../common.h"

// A widget to cycle through several pages of a text content.

typedef struct {
    char *title;
    size_t max_lines;
    int32_t current_page;
    VECTOR *page_content;
    bool is_empty;
    bool is_heavy;
} UI_TEXT_DIALOG_STATE;

void UI_TextDialog_Init(
    UI_TEXT_DIALOG_STATE *state, const char *title, const char *text,
    size_t width, size_t max_lines, bool is_heavy);
void UI_TextDialog_Control(UI_TEXT_DIALOG_STATE *state);
void UI_TextDialog_Free(UI_TEXT_DIALOG_STATE *state);

void UI_TextDialog(UI_TEXT_DIALOG_STATE *state);
