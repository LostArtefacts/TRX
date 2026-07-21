#pragma once

#include <trx/core/completion.h>
#include <trx/game/ui/common.h>
#include <trx/game/ui/elements/flash.h>

#include <stddef.h>
#include <stdint.h>

// A text edit widget that collects text input from the player.
// Needs to be in focus to work, otherwise is inactive.

// The live Tab-completion session for one prompt: the provider that picks a
// completer, the last query's result, and where the cycle sits in it.
typedef struct {
    COMPLETER_PROVIDER provider;
    // The span and suggestions from the last query; empty while no session is
    // open.
    COMPLETION result;
    // Active suggestion, or -1 when no session is open. One past the last
    // suggestion selects `original`.
    int32_t index;
    // The run the player typed before cycling, restored at that wrap slot.
    // nullptr while no session is open.
    char *original;
} UI_PROMPT_COMPLETION;

typedef struct {
    bool is_focused;
    int32_t caret_pos;
    int32_t current_text_capacity;
    char *current_text;

    int32_t listener1;
    int32_t listener2;

    UI_FLASH_STATE flash;

    UI_PROMPT_COMPLETION completion;
} UI_PROMPT_STATE;

// state functions
void UI_Prompt_Init(UI_PROMPT_STATE *s);
void UI_Prompt_Free(UI_PROMPT_STATE *s);
void UI_Prompt_Control(UI_PROMPT_STATE *s);
void UI_Prompt_Clear(UI_PROMPT_STATE *s);
void UI_Prompt_SetFocus(UI_PROMPT_STATE *s, bool is_focused);
void UI_Prompt_ChangeText(UI_PROMPT_STATE *s, const char *new_text);
void UI_Prompt_SetCompletionProvider(
    UI_PROMPT_STATE *s, COMPLETER_PROVIDER provider);

// draw functions
void UI_Prompt(UI_PROMPT_STATE *s);
