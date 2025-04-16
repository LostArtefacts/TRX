#pragma once

// A control backend (keyboard/controller) choice dialog.

#include "../common.h"
#include "../elements/requester.h"

typedef struct {
    UI2_REQUESTER_STATE req;
} UI2_CONTROLS_BACKEND_STATE;

// state functions
void UI2_ControlsBackend_Init(UI2_CONTROLS_BACKEND_STATE *s);
void UI2_ControlsBackend_Free(UI2_CONTROLS_BACKEND_STATE *s);
int32_t UI2_ControlsBackend_Control(UI2_CONTROLS_BACKEND_STATE *s);

// draw functions
void UI2_ControlsBackend(UI2_CONTROLS_BACKEND_STATE *s);
