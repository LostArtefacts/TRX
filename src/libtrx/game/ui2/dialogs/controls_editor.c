#include "game/ui2/dialogs/controls_editor.h"

#include "config.h"
#include "game/const.h"
#include "game/game_string.h"
#include "game/input.h"
#include "game/shell.h"
#include "game/ui2/elements/anchor.h"
#include "game/ui2/elements/frame.h"
#include "game/ui2/elements/label.h"
#include "game/ui2/elements/modal.h"
#include "game/ui2/elements/pad.h"
#include "game/ui2/elements/requester.h"
#include "game/ui2/elements/spacer.h"
#include "game/ui2/elements/stack.h"
#include "game/ui2/elements/window.h"
#include "utils.h"

typedef enum {
    M_PHASE_NAVIGATE_LAYOUT,
    M_PHASE_NAVIGATE_INPUTS,
    M_PHASE_NAVIGATE_INPUTS_DEBOUNCE,
    M_PHASE_LISTEN,
    M_PHASE_LISTEN_DEBOUNCE,
    M_PHASE_EXIT,
} M_PHASE;

static const INPUT_ROLE m_LeftRoles[] = {
    // clang-format off
    INPUT_ROLE_UP,
    INPUT_ROLE_DOWN,
    INPUT_ROLE_LEFT,
    INPUT_ROLE_RIGHT,
    INPUT_ROLE_STEP_L,
    INPUT_ROLE_STEP_R,
    INPUT_ROLE_SLOW,
    INPUT_ROLE_ENTER_CONSOLE,
    INPUT_ROLE_PAUSE,
    INPUT_ROLE_TOGGLE_PHOTO_MODE,
    INPUT_ROLE_TOGGLE_UI,
    // INPUT_ROLE_CAMERA_RESET, // same as look, no need to configure
    INPUT_ROLE_CAMERA_UP,
    INPUT_ROLE_CAMERA_DOWN,
    INPUT_ROLE_CAMERA_LEFT,
    INPUT_ROLE_CAMERA_RIGHT,
    INPUT_ROLE_CAMERA_FORWARD,
    INPUT_ROLE_CAMERA_BACK,
    (INPUT_ROLE)-1,
    // clang-format on
};

static const INPUT_ROLE m_RightRoles_CheatsOff[] = {
    // clang-format off
    INPUT_ROLE_JUMP,
    INPUT_ROLE_ACTION,
    INPUT_ROLE_DRAW,
#if TR_VERSION == 2
    INPUT_ROLE_USE_FLARE,
#endif
    INPUT_ROLE_LOOK,
    INPUT_ROLE_ROLL,
    INPUT_ROLE_OPTION,
    (INPUT_ROLE)-1,
    // clang-format on
};

static const INPUT_ROLE m_RightRoles_CheatsOn[] = {
    // clang-format off
    INPUT_ROLE_JUMP,
    INPUT_ROLE_ACTION,
    INPUT_ROLE_DRAW,
#if TR_VERSION == 2
    INPUT_ROLE_USE_FLARE,
#endif
    INPUT_ROLE_LOOK,
    INPUT_ROLE_ROLL,
    INPUT_ROLE_OPTION,
    INPUT_ROLE_FLY_CHEAT,
    INPUT_ROLE_ITEM_CHEAT,
    INPUT_ROLE_LEVEL_SKIP_CHEAT,
    INPUT_ROLE_TURBO_CHEAT,
    (INPUT_ROLE)-1,
    // clang-format on
};

static const INPUT_ROLE *m_RightRoles = nullptr;

static INPUT_ROLE M_GetInputRole(int32_t col, int32_t row);
static int32_t M_GetInputRoleCount(int32_t col);
static void M_CycleLayout(UI2_CONTROLS_EDITOR_STATE *s, int32_t dir);
static UI2_CONTROLS_CHOICE M_NavigateLayout(UI2_CONTROLS_EDITOR_STATE *s);
static UI2_CONTROLS_CHOICE M_NavigateInputs(UI2_CONTROLS_EDITOR_STATE *s);
static UI2_CONTROLS_CHOICE M_NavigateInputsDebounce(
    UI2_CONTROLS_EDITOR_STATE *s);
static UI2_CONTROLS_CHOICE M_Listen(UI2_CONTROLS_EDITOR_STATE *s);
static UI2_CONTROLS_CHOICE M_ListenDebounce(UI2_CONTROLS_EDITOR_STATE *s);

static void M_Title(const UI2_CONTROLS_EDITOR_STATE *s);
static void M_InputChoice(UI2_CONTROLS_EDITOR_STATE *s, INPUT_ROLE role);
static void M_InputLabel(const UI2_CONTROLS_EDITOR_STATE *s, INPUT_ROLE role);
static void M_Column(UI2_CONTROLS_EDITOR_STATE *s, const INPUT_ROLE *roles);

static INPUT_ROLE M_GetInputRole(const int32_t col, const int32_t row)
{
    if (col == 0) {
        return m_LeftRoles[row];
    } else {
        return m_RightRoles[row];
    }
}

static int32_t M_GetInputRoleCount(const int32_t col)
{
    int32_t row = 0;
    while (M_GetInputRole(col, row) != (INPUT_ROLE)-1) {
        row++;
    }
    return row;
}

static void M_CycleLayout(UI2_CONTROLS_EDITOR_STATE *const s, const int32_t dir)
{
    s->active_layout += dir;
    s->active_layout += INPUT_LAYOUT_NUMBER_OF;
    s->active_layout %= INPUT_LAYOUT_NUMBER_OF;

    const EVENT event = {
        .name = "layout_change",
        .sender = nullptr,
        .data = nullptr,
    };
    EventManager_Fire(s->events, &event);
}

static UI2_CONTROLS_CHOICE M_NavigateLayout(UI2_CONTROLS_EDITOR_STATE *const s)
{
    if (g_InputDB.menu_confirm) {
        return UI2_CONTROLS_CHOICE_EXIT;
    } else if (g_InputDB.menu_back) {
        return UI2_CONTROLS_CHOICE_GO_BACK;
    } else if (g_InputDB.menu_left) {
        M_CycleLayout(s, -1);
    } else if (g_InputDB.menu_right) {
        M_CycleLayout(s, 1);
    } else if (g_InputDB.menu_down && s->active_layout != 0) {
        s->phase = M_PHASE_NAVIGATE_INPUTS;
        s->active_col = 0;
        s->active_row = 0;
    } else if (g_InputDB.menu_up && s->active_layout != 0) {
        s->phase = M_PHASE_NAVIGATE_INPUTS;
        s->active_col = 1;
        s->active_row = M_GetInputRoleCount(1) - 1;
    } else {
        return UI2_CONTROLS_CHOICE_NOOP;
    }
    s->active_role = M_GetInputRole(s->active_col, s->active_row);
    return UI2_CONTROLS_CHOICE_NOOP;
}

static UI2_CONTROLS_CHOICE M_NavigateInputs(UI2_CONTROLS_EDITOR_STATE *const s)
{
    if (g_InputDB.menu_confirm) {
        s->phase = M_PHASE_NAVIGATE_INPUTS_DEBOUNCE;
    } else if (g_InputDB.menu_back) {
        return UI2_CONTROLS_CHOICE_GO_BACK;
    } else if (g_InputDB.menu_left || g_InputDB.menu_right) {
        s->active_col ^= 1;
        CLAMP(s->active_row, 0, M_GetInputRoleCount(s->active_col) - 1);
    } else if (g_InputDB.menu_up) {
        s->active_row--;
        if (s->active_row < 0) {
            if (s->active_col == 0) {
                s->phase = M_PHASE_NAVIGATE_LAYOUT;
            } else {
                s->active_col = 0;
                s->active_row = M_GetInputRoleCount(0) - 1;
            }
        }
    } else if (g_InputDB.menu_down) {
        s->active_row++;
        if (s->active_row >= M_GetInputRoleCount(s->active_col)) {
            if (s->active_col == 0) {
                s->active_col = 1;
                s->active_row = 0;
            } else {
                s->phase = M_PHASE_NAVIGATE_LAYOUT;
            }
        }
    } else {
        return UI2_CONTROLS_CHOICE_NOOP;
    }
    s->active_role = M_GetInputRole(s->active_col, s->active_row);
    return UI2_CONTROLS_CHOICE_NOOP;
}

static UI2_CONTROLS_CHOICE M_NavigateInputsDebounce(
    UI2_CONTROLS_EDITOR_STATE *const s)
{
    Shell_ProcessEvents();
    Input_Update();
    if (g_Input.any) {
        return UI2_CONTROLS_CHOICE_NOOP;
    }
    Input_EnterListenMode();
    s->phase = M_PHASE_LISTEN;
    return UI2_CONTROLS_CHOICE_NOOP;
}

static UI2_CONTROLS_CHOICE M_Listen(UI2_CONTROLS_EDITOR_STATE *const s)
{
    if (!Input_ReadAndAssignRole(
            s->backend, s->active_layout, s->active_role)) {
        return UI2_CONTROLS_CHOICE_NOOP;
    }

    Input_ExitListenMode();

    const EVENT event = {
        .name = "key_change",
        .sender = nullptr,
        .data = nullptr,
    };
    EventManager_Fire(s->events, &event);

    s->phase = M_PHASE_LISTEN_DEBOUNCE;
    return UI2_CONTROLS_CHOICE_NOOP;
}

static UI2_CONTROLS_CHOICE M_ListenDebounce(UI2_CONTROLS_EDITOR_STATE *const s)
{
    if (!g_Input.any) {
        s->phase = M_PHASE_NAVIGATE_INPUTS;
    }
    return UI2_CONTROLS_CHOICE_NOOP;
}

static void M_Title(const UI2_CONTROLS_EDITOR_STATE *const s)
{
    UI2_BeginAnchor(0.5f, 0.5f);
    if (s->phase == M_PHASE_NAVIGATE_LAYOUT) {
        UI2_BeginFrame(UI2_FRAME_SELECTED_OPTION);
    }
    UI2_BeginPad(2.0f, 1.0f);
    UI2_Label(Input_GetLayoutName(s->active_layout));
    UI2_EndPad();
    if (s->phase == M_PHASE_NAVIGATE_LAYOUT) {
        UI2_EndFrame();
    }
    UI2_EndAnchor();
}

static void M_InputLabel(
    const UI2_CONTROLS_EDITOR_STATE *const s, const INPUT_ROLE role)
{
    const bool is_selected = s->active_role == role
        && (s->phase == M_PHASE_NAVIGATE_INPUTS
            || s->phase == M_PHASE_NAVIGATE_INPUTS_DEBOUNCE);
    if (is_selected) {
        UI2_BeginFrame(UI2_FRAME_SELECTED_OPTION);
    }
    UI2_Label(Input_GetRoleName(role));
    if (is_selected) {
        UI2_EndFrame();
    }
}

static void M_InputChoice(
    UI2_CONTROLS_EDITOR_STATE *const s, const INPUT_ROLE role)
{
    const bool is_flashing =
        Input_IsKeyConflicted(s->backend, s->active_layout, role);
    const bool is_selected =
        s->active_role == role && s->phase == M_PHASE_LISTEN;

    if (is_flashing) {
        UI2_BeginFlash(&s->flash);
    }
    if (is_selected) {
        UI2_BeginFrame(UI2_FRAME_SELECTED_OPTION);
    }
    UI2_Label(Input_GetKeyName(s->backend, s->active_layout, role));
    if (is_selected) {
        UI2_EndFrame();
    }
    if (is_flashing) {
        UI2_EndFlash();
    }
}

static void M_Column(
    UI2_CONTROLS_EDITOR_STATE *const s, const INPUT_ROLE *const roles)
{
    UI2_BeginStack(UI2_STACK_HORIZONTAL);
    UI2_BeginStack(UI2_STACK_VERTICAL);
    for (const INPUT_ROLE *role = roles; *role != (INPUT_ROLE)-1; role++) {
        M_InputChoice(s, *role);
    }
    UI2_EndStack();
    UI2_Spacer(10.0f, 0.0f);
    UI2_BeginStack(UI2_STACK_VERTICAL);
    for (const INPUT_ROLE *role = roles; *role != (INPUT_ROLE)-1; role++) {
        M_InputLabel(s, *role);
    }
    UI2_EndStack();
    UI2_EndStack();
}

void UI2_ControlsEditor_Init(
    UI2_CONTROLS_EDITOR_STATE *const s, EVENT_MANAGER *events)
{
    m_RightRoles = g_Config.gameplay.enable_cheats ? m_RightRoles_CheatsOn
                                                   : m_RightRoles_CheatsOff;
    s->events = events;
    UI2_Flash_Init(&s->flash, LOGIC_FPS * 2 / 3);
}

void UI2_ControlsEditor_Free(UI2_CONTROLS_EDITOR_STATE *const s)
{
    UI2_Flash_Free(&s->flash);
}

void UI2_ControlsEditor_Reinit(
    UI2_CONTROLS_EDITOR_STATE *s, INPUT_BACKEND backend, int32_t layout)
{
    s->backend = backend;
    s->active_layout = layout;
    s->active_row = 0;
    s->active_col = 0;
    s->active_role = M_GetInputRole(s->active_col, s->active_row);
    s->phase = M_PHASE_NAVIGATE_LAYOUT;
}

UI2_CONTROLS_CHOICE UI2_ControlsEditor_Control(
    UI2_CONTROLS_EDITOR_STATE *const s)
{
    UI2_Flash_Control(&s->flash);
    switch (s->phase) {
    case M_PHASE_NAVIGATE_LAYOUT:
        return M_NavigateLayout(s);
    case M_PHASE_NAVIGATE_INPUTS:
        return M_NavigateInputs(s);
    case M_PHASE_NAVIGATE_INPUTS_DEBOUNCE:
        return M_NavigateInputsDebounce(s);
    case M_PHASE_LISTEN:
        return M_Listen(s);
    case M_PHASE_LISTEN_DEBOUNCE:
        return M_ListenDebounce(s);
    default:
        return UI2_CONTROLS_CHOICE_NOOP;
    }
}

void UI2_ControlsEditor(UI2_CONTROLS_EDITOR_STATE *const s)
{
    UI2_BeginModal(0.5f, 0.5f);
    UI2_BeginWindow();
    UI2_WindowTitle(GS(CONTROLS_CUSTOMIZE));
    UI2_BeginWindowBody();

    UI2_BeginStackEx((UI2_STACK_SETTINGS) {
        .orientation = UI2_STACK_VERTICAL,
        .align = { .h = UI2_STACK_H_ALIGN_SPAN },
    });
    M_Title(s);
    UI2_Spacer(0.0f, 5.0f);

    UI2_BeginStack(UI2_STACK_HORIZONTAL);
    M_Column(s, m_LeftRoles);
    UI2_Spacer(10.0f, 0.0f);
    M_Column(s, m_RightRoles);
    UI2_EndStack();

    UI2_EndStack();
    UI2_EndWindowBody();
    UI2_EndWindow();
    UI2_EndModal();
}
