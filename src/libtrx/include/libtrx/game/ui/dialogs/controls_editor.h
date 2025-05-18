#pragma once

// A controls remapper dialog.

#include "../../../event_manager.h"
#include "../../input.h"
#include "../common.h"
#include "../elements/flash.h"
#include "../elements/requester.h"

typedef struct {
    int32_t phase;
    INPUT_BACKEND backend;
    int32_t active_layout;
    INPUT_ROLE active_role;
    int32_t active_col;
    int32_t active_row;
    UI_FLASH_STATE flash;
    EVENT_MANAGER *events;
    int32_t hold_timer;
} UI_CONTROLS_EDITOR_STATE;

typedef enum {
    UI_CONTROLS_CHOICE_EXIT,
    UI_CONTROLS_CHOICE_GO_BACK,
    UI_CONTROLS_CHOICE_NOOP,
} UI_CONTROLS_CHOICE;

// state functions
void UI_ControlsEditor_Init(UI_CONTROLS_EDITOR_STATE *s, EVENT_MANAGER *events);
void UI_ControlsEditor_Free(UI_CONTROLS_EDITOR_STATE *s);
void UI_ControlsEditor_Reinit(
    UI_CONTROLS_EDITOR_STATE *s, INPUT_BACKEND backend, int32_t layout);
UI_CONTROLS_CHOICE UI_ControlsEditor_Control(UI_CONTROLS_EDITOR_STATE *s);

// draw functions
void UI_ControlsEditor(UI_CONTROLS_EDITOR_STATE *s);
