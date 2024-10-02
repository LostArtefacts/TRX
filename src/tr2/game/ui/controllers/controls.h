#pragma once

#include "game/input.h"

typedef enum {
    UI_CONTROLS_STATE_NAVIGATE_LAYOUT,
    UI_CONTROLS_STATE_NAVIGATE_INPUTS,
    UI_CONTROLS_STATE_LISTEN_DEBOUNCE,
    UI_CONTROLS_STATE_LISTEN,
    UI_CONTROLS_STATE_NAVIGATE_INPUTS_DEBOUNCE,
    UI_CONTROLS_STATE_EXIT,
} UI_CONTROLS_STATE;

typedef struct {
    int32_t active_layout;
    int32_t active_col;
    int32_t active_row;
    INPUT_ROLE active_role;
    UI_CONTROLS_STATE state;
} UI_CONTROLS_CONTROLLER;

bool UI_ControlsController_Control(UI_CONTROLS_CONTROLLER *controller);

INPUT_ROLE UI_ControlsController_GetInputRole(int32_t col, int32_t row);
int32_t UI_ControlsController_GetInputRoleCount(int32_t col);
