#pragma once

// A control backend (keyboard/controller/touch) choice dialog.

#include <trx/game/ui/common.h>
#include <trx/game/ui/elements/requester.h>

#define UI_CONTROLS_BACKEND_MAX_OPTIONS 8

typedef struct {
    UI_REQUESTER_STATE req;
    int32_t visible_indices[UI_CONTROLS_BACKEND_MAX_OPTIONS];
} UI_CONTROLS_BACKEND_STATE;

// state functions
void UI_ControlsBackend_Init(UI_CONTROLS_BACKEND_STATE *s);
void UI_ControlsBackend_Free(UI_CONTROLS_BACKEND_STATE *s);
int32_t UI_ControlsBackend_Control(UI_CONTROLS_BACKEND_STATE *s);

// draw functions
void UI_ControlsBackend(UI_CONTROLS_BACKEND_STATE *s);
