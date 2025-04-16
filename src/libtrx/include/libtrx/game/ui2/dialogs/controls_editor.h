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
    UI2_FLASH_STATE flash;
    EVENT_MANAGER *events;
} UI2_CONTROLS_EDITOR_STATE;

typedef enum {
    UI2_CONTROLS_CHOICE_EXIT,
    UI2_CONTROLS_CHOICE_GO_BACK,
    UI2_CONTROLS_CHOICE_NOOP,
} UI2_CONTROLS_CHOICE;

// state functions
void UI2_ControlsEditor_Init(
    UI2_CONTROLS_EDITOR_STATE *s, EVENT_MANAGER *events);
void UI2_ControlsEditor_Free(UI2_CONTROLS_EDITOR_STATE *s);
void UI2_ControlsEditor_Reinit(
    UI2_CONTROLS_EDITOR_STATE *s, INPUT_BACKEND backend, int32_t layout);
UI2_CONTROLS_CHOICE UI2_ControlsEditor_Control(UI2_CONTROLS_EDITOR_STATE *s);

// draw functions
void UI2_ControlsEditor(UI2_CONTROLS_EDITOR_STATE *s);
