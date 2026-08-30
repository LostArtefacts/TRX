#pragma once

#include <trx/core/result.h>

#include <stdint.h>

typedef enum {
    UI_KEY_UP,
    UI_KEY_DOWN,
    UI_KEY_LEFT,
    UI_KEY_RIGHT,
    UI_KEY_HOME,
    UI_KEY_END,
    UI_KEY_BACK,
    UI_KEY_RETURN,
    UI_KEY_ESCAPE,
    UI_KEY_TAB,
    UI_KEY_SHIFT_TAB,
} UI_INPUT;

void UI_HandleKeyDown(uint32_t key);
void UI_HandleKeyUp(uint32_t key);
void UI_HandleTextEdit(const char *text);

// Inserts the current clipboard contents (if any) into the currently
// focused text field, as if it had been typed.
void UI_HandlePaste(void);

// Puts text in the system clipboard and reports failure if the platform refuses
// it.
RESULT UI_SetClipboardText(const char *text);
