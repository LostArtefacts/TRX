#pragma once

#include "../../../vector.h"
#include "../common.h"

// A widget to cycle through several pages of a text content.

typedef struct UI_TEXT_DIALOG_STATE UI_TEXT_DIALOG_STATE;

// state functions
UI_TEXT_DIALOG_STATE *UI_TextDialog_Init(
    size_t wrap_width, size_t wrap_max_lines, bool is_heavy);

// Handle page-left/right input.  Call before UI_TextDialog().
void UI_TextDialog_Control(UI_TEXT_DIALOG_STATE *state);

// Free any allocated buffers.  Call when dialog is dismissed.
void UI_TextDialog_Free(UI_TEXT_DIALOG_STATE *state);

// draw functions

// Draw and manage a text dialog in one call.  Rewraps/recapitalizes only
// when title_raw/text_raw differ (compared via strcmp).  Call every frame.
void UI_TextDialog(
    UI_TEXT_DIALOG_STATE *state, const char *title_raw, const char *text_raw);
