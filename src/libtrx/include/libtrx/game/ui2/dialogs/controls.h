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
    UI2_CONTROLS_BACKEND_STATE backend_state;
    UI2_CONTROLS_EDITOR_STATE editor_state;
} UI2_CONTROLS_STATE;

// state functions
void UI2_Controls_Init(UI2_CONTROLS_STATE *s);
void UI2_Controls_Free(UI2_CONTROLS_STATE *s);
bool UI2_Controls_Control(UI2_CONTROLS_STATE *s);

// draw functions
void UI2_Controls(UI2_CONTROLS_STATE *s);
