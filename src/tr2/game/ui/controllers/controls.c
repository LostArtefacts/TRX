#include "game/ui/controllers/controls.h"

#include "game/input.h"
#include "global/vars.h"

#include <libtrx/utils.h>

#include <dinput.h>

static const INPUT_ROLE m_LeftRoles[] = {
    INPUT_ROLE_UP,    INPUT_ROLE_DOWN,      INPUT_ROLE_LEFT,
    INPUT_ROLE_RIGHT, INPUT_ROLE_STEP_LEFT, INPUT_ROLE_STEP_RIGHT,
    INPUT_ROLE_SLOW,  (INPUT_ROLE)-1,
};

static const INPUT_ROLE m_RightRoles[] = {
    INPUT_ROLE_JUMP,   INPUT_ROLE_ACTION,  INPUT_ROLE_DRAW_WEAPON,
    INPUT_ROLE_FLARE,  INPUT_ROLE_LOOK,    INPUT_ROLE_ROLL,
    INPUT_ROLE_OPTION, INPUT_ROLE_CONSOLE, (INPUT_ROLE)-1,
};

static const INPUT_ROLE *M_GetInputRoles(int32_t col);
static bool M_NavigateLayout(UI_CONTROLS_CONTROLLER *controller);
static bool M_NavigateInputs(UI_CONTROLS_CONTROLLER *controller);
static bool M_ListenDebounce(UI_CONTROLS_CONTROLLER *controller);
static bool M_Listen(UI_CONTROLS_CONTROLLER *controller);
static bool M_NavigateInputsDebounce(UI_CONTROLS_CONTROLLER *controller);

static const INPUT_ROLE *M_GetInputRoles(const int32_t col)
{
    return col == 0 ? m_LeftRoles : m_RightRoles;
}

static bool M_NavigateLayout(UI_CONTROLS_CONTROLLER *const controller)
{
    if ((g_InputDB & IN_DESELECT) || (g_InputDB & IN_SELECT)) {
        controller->state = UI_CONTROLS_STATE_EXIT;
    } else if (g_InputDB & IN_RIGHT) {
        controller->active_layout++;
        controller->active_layout %= INPUT_MAX_LAYOUT;
    } else if (g_InputDB & IN_LEFT) {
        if (controller->active_layout == 0) {
            controller->active_layout = INPUT_MAX_LAYOUT - 1;
        } else {
            controller->active_layout--;
        }
    } else if ((g_InputDB & IN_BACK) && controller->active_layout != 0) {
        controller->state = UI_CONTROLS_STATE_NAVIGATE_INPUTS;
        controller->active_col = 0;
        controller->active_row = 0;
    } else if ((g_InputDB & IN_FORWARD) && controller->active_layout != 0) {
        controller->state = UI_CONTROLS_STATE_NAVIGATE_INPUTS;
        controller->active_col = 1;
        controller->active_row = UI_ControlsController_GetInputRoleCount(1) - 1;
    } else {
        return false;
    }
    controller->active_role =
        M_GetInputRoles(controller->active_col)[controller->active_row];
    return true;
}

static bool M_NavigateInputs(UI_CONTROLS_CONTROLLER *const controller)
{
    if (g_InputDB & IN_DESELECT) {
        controller->state = UI_CONTROLS_STATE_EXIT;
    } else if (g_InputDB & (IN_LEFT | IN_RIGHT)) {
        controller->active_col ^= 1;
        CLAMP(
            controller->active_row, 0,
            UI_ControlsController_GetInputRoleCount(controller->active_col)
                - 1);
    } else if (g_InputDB & IN_FORWARD) {
        controller->active_row--;
        if (controller->active_row < 0) {
            if (controller->active_col == 0) {
                controller->state = UI_CONTROLS_STATE_NAVIGATE_LAYOUT;
            } else {
                controller->active_col = 0;
                controller->active_row =
                    UI_ControlsController_GetInputRoleCount(0) - 1;
            }
        }
    } else if (g_InputDB & IN_BACK) {
        controller->active_row++;
        if (controller->active_row >= UI_ControlsController_GetInputRoleCount(
                controller->active_col)) {
            if (controller->active_col == 0) {
                controller->active_col = 1;
                controller->active_row = 0;
            } else {
                controller->state = UI_CONTROLS_STATE_NAVIGATE_LAYOUT;
            }
        }
    } else if (g_InputDB & IN_SELECT) {
        controller->state = UI_CONTROLS_STATE_LISTEN_DEBOUNCE;
    } else {
        return false;
    }
    controller->active_role =
        M_GetInputRoles(controller->active_col)[controller->active_row];
    return true;
}

static bool M_ListenDebounce(UI_CONTROLS_CONTROLLER *const controller)
{
    if (Input_IsAnythingPressed()) {
        return false;
    }
    Input_EnterListenMode();
    controller->state = UI_CONTROLS_STATE_LISTEN;
    return true;
}

static bool M_Listen(UI_CONTROLS_CONTROLLER *const controller)
{
    int32_t pressed = 0;

    if (g_JoyKeys != 0) {
        for (int32_t i = 0; i < 32; i++) {
            if (g_JoyKeys & (1 << i)) {
                pressed = i;
                break;
            }
        }
        if (!pressed) {
            return false;
        }
        pressed += 0x100;
    } else {
        for (int32_t i = 0; i < 256; i++) {
            if (g_DIKeys[i] & 0x80) {
                pressed = i;
                break;
            }
        }
        if (!pressed) {
            return false;
        }
    }

    if (!pressed
        // clang-format off
        || Input_GetKeyName(pressed) == NULL
        || pressed == DIK_RETURN
        || pressed == DIK_LEFT
        || pressed == DIK_RIGHT
        || pressed == DIK_UP
        || pressed == DIK_DOWN
        // clang-format on
    ) {
        g_Input = 0;
        g_InputDB = 0;
        return false;
    }

    if (pressed != DIK_ESCAPE) {
        Input_AssignKey(
            controller->active_layout, controller->active_role, pressed);
    }

    controller->state = UI_CONTROLS_STATE_NAVIGATE_INPUTS_DEBOUNCE;
    return true;
}

static bool M_NavigateInputsDebounce(UI_CONTROLS_CONTROLLER *const controller)
{
    if (Input_IsAnythingPressed()) {
        return false;
    }

    Input_ExitListenMode();
    controller->state = UI_CONTROLS_STATE_NAVIGATE_INPUTS;
    return true;
}

bool UI_ControlsController_Control(UI_CONTROLS_CONTROLLER *const controller)
{
    switch (controller->state) {
    case UI_CONTROLS_STATE_NAVIGATE_LAYOUT:
        return M_NavigateLayout(controller);
    case UI_CONTROLS_STATE_NAVIGATE_INPUTS:
        return M_NavigateInputs(controller);
    case UI_CONTROLS_STATE_LISTEN_DEBOUNCE:
        return M_ListenDebounce(controller);
    case UI_CONTROLS_STATE_LISTEN:
        return M_Listen(controller);
    case UI_CONTROLS_STATE_NAVIGATE_INPUTS_DEBOUNCE:
        return M_NavigateInputsDebounce(controller);
    default:
        return false;
    }
    return false;
}

INPUT_ROLE UI_ControlsController_GetInputRole(
    const int32_t col, const int32_t row)
{
    return M_GetInputRoles(col)[row];
}

int32_t UI_ControlsController_GetInputRoleCount(const int32_t col)
{
    int32_t result = 0;
    const INPUT_ROLE *const roles = M_GetInputRoles(col);
    while (roles[result] != (INPUT_ROLE)-1) {
        result++;
    }
    return result;
}
