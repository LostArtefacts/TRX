#pragma once

// A controls editor dialog.

#include "../../../event_manager.h"
#include "../../input.h"
#include "../common.h"
#include "./controls_backend.h"
#include "./controls_editor.h"

typedef struct {
    int32_t phase;
    INPUT_BACKEND backend;
    int32_t active_layout;

    EVENT_MANAGER *events;
    UI_CONTROLS_BACKEND_STATE backend_state;
    UI_CONTROLS_EDITOR_STATE editor_state;
} UI_CONTROLS_STATE;

// state functions
void UI_Controls_Init(UI_CONTROLS_STATE *s);
void UI_Controls_Free(UI_CONTROLS_STATE *s);
bool UI_Controls_Control(UI_CONTROLS_STATE *s);

// draw functions
void UI_Controls(UI_CONTROLS_STATE *s);
