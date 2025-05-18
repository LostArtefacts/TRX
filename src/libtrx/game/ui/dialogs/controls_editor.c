#include "game/ui/dialogs/controls_editor.h"

#include "config.h"
#include "game/const.h"
#include "game/game_string.h"
#include "game/input.h"
#include "game/scaler.h"
#include "game/shell.h"
#include "game/sound.h"
#include "game/text.h"
#include "game/ui/elements/anchor.h"
#include "game/ui/elements/bar.h"
#include "game/ui/elements/frame.h"
#include "game/ui/elements/hide.h"
#include "game/ui/elements/label.h"
#include "game/ui/elements/modal.h"
#include "game/ui/elements/pad.h"
#include "game/ui/elements/requester.h"
#include "game/ui/elements/resize.h"
#include "game/ui/elements/spacer.h"
#include "game/ui/elements/span.h"
#include "game/ui/elements/stack.h"
#include "game/ui/elements/window.h"
#include "game/viewport.h"
#include "utils.h"

#define M_HOLD_TIMER_DEBUFF (LOGIC_FPS / 3)
#define M_HOLD_TIMER_MAX LOGIC_FPS

typedef enum {
    M_PHASE_NAVIGATE_LAYOUT,
    M_PHASE_NAVIGATE_GROUP,
    M_PHASE_NAVIGATE_INPUTS,
    M_PHASE_NAVIGATE_INPUTS_DEBOUNCE,
    M_PHASE_LISTEN,
    M_PHASE_LISTEN_DEBOUNCE,
    M_PHASE_EXIT,
} M_PHASE;

typedef void (*M_HOLD_ACTION_FUNC)(const UI_CONTROLS_EDITOR_STATE *);

static const UI_CONTROLS_EDITOR_GROUP m_Groups[] = {
    {
        .header = GS_ID(CONTROLS_SECTION_BASICS),
        .roles =
            (INPUT_ROLE[]) {
                INPUT_ROLE_UP,
                INPUT_ROLE_DOWN,
                INPUT_ROLE_LEFT,
                INPUT_ROLE_RIGHT,
                INPUT_ROLE_JUMP,
                INPUT_ROLE_STEP_L,
                INPUT_ROLE_STEP_R,
                INPUT_ROLE_ROLL,
                INPUT_ROLE_SLOW,
                INPUT_ROLE_ACTION,
                INPUT_ROLE_DRAW,
                INPUT_ROLE_LOOK,
                (INPUT_ROLE)-1,
            },
    },
    {
        .header = GS_ID(CONTROLS_SECTION_ITEMS),
        .roles =
            (INPUT_ROLE[]) {
#if TR_VERSION == 2
                INPUT_ROLE_USE_FLARE,
#endif
                INPUT_ROLE_USE_SMALL_MEDI,
                INPUT_ROLE_USE_BIG_MEDI,
                INPUT_ROLE_EQUIP_PISTOLS,
                INPUT_ROLE_EQUIP_SHOTGUN,
                INPUT_ROLE_EQUIP_MAGNUMS,
                INPUT_ROLE_EQUIP_UZIS,
#if TR_VERSION == 2
                INPUT_ROLE_EQUIP_HARPOON,
                INPUT_ROLE_EQUIP_M16,
                INPUT_ROLE_EQUIP_GRENADE_LAUNCHER,
#endif
                (INPUT_ROLE)-1,
            },
    },

    {
        .header = GS_ID(CONTROLS_SECTION_MISC),
        .roles =
            (INPUT_ROLE[]) {
#if TR_VERSION == 1
                INPUT_ROLE_CHANGE_TARGET,
#endif
                INPUT_ROLE_CAMERA_UP,
                INPUT_ROLE_CAMERA_DOWN,
                INPUT_ROLE_CAMERA_LEFT,
                INPUT_ROLE_CAMERA_RIGHT,
                INPUT_ROLE_CAMERA_FORWARD,
                INPUT_ROLE_CAMERA_BACK,
                INPUT_ROLE_FLY_CHEAT,
                INPUT_ROLE_ITEM_CHEAT,
                INPUT_ROLE_LEVEL_SKIP_CHEAT,
                INPUT_ROLE_TURBO_CHEAT,
                (INPUT_ROLE)-1,
            },
    },

    {
        .header = GS_ID(CONTROLS_SECTION_SYSTEM),
        .roles =
            (INPUT_ROLE[]) {
                INPUT_ROLE_OPTION,
                INPUT_ROLE_SAVE,
                INPUT_ROLE_LOAD,
                INPUT_ROLE_PAUSE,
                // INPUT_ROLE_SCREENSHOT, // handled specially
                INPUT_ROLE_FPS,
                // INPUT_ROLE_TOGGLE_FULLSCREEN, // handled specially
                INPUT_ROLE_ENTER_CONSOLE,
                INPUT_ROLE_TOGGLE_PHOTO_MODE,
                INPUT_ROLE_TOGGLE_UI,
#if TR_VERSION == 1
                INPUT_ROLE_BILINEAR,
#elif TR_VERSION == 2
                INPUT_ROLE_TOGGLE_BILINEAR_FILTER,
                // INPUT_ROLE_TOGGLE_PERSPECTIVE_FILTER, // handled specially
                INPUT_ROLE_TOGGLE_TRAPEZOID_FILTER,
                INPUT_ROLE_SWITCH_INTERNAL_SCREEN_SIZE,
                INPUT_ROLE_SWITCH_RESOLUTION,
                INPUT_ROLE_TOGGLE_Z_BUFFER,
                INPUT_ROLE_CYCLE_LIGHTING_CONTRAST,
                INPUT_ROLE_TOGGLE_RENDERING_MODE,
#endif
                (INPUT_ROLE)-1,
            },
    },

    {
        .header = nullptr,
    },
};

static int32_t M_GetVisibleRows(void);

static INPUT_ROLE M_GetInputRole(
    const UI_CONTROLS_EDITOR_GROUP *group, int32_t row);
static int32_t M_GetInputRoleCount(const UI_CONTROLS_EDITOR_GROUP *group);
static void M_CycleLayout(UI_CONTROLS_EDITOR_STATE *s, int32_t dir);
static void M_CycleGroup(UI_CONTROLS_EDITOR_STATE *s, int32_t dir);
static void M_ResetLayout(const UI_CONTROLS_EDITOR_STATE *s);
static void M_UnbindKey(const UI_CONTROLS_EDITOR_STATE *s);
static bool M_HandleHoldAction(
    UI_CONTROLS_EDITOR_STATE *s, INPUT_ROLE role,
    M_HOLD_ACTION_FUNC action_func);
static void M_CheckResetKeys(UI_CONTROLS_EDITOR_STATE *s);
static UI_CONTROLS_CHOICE M_NavigateLayout(UI_CONTROLS_EDITOR_STATE *s);
static UI_CONTROLS_CHOICE M_NavigateGroup(UI_CONTROLS_EDITOR_STATE *s);
static UI_CONTROLS_CHOICE M_NavigateInputs(UI_CONTROLS_EDITOR_STATE *s);
static UI_CONTROLS_CHOICE M_NavigateInputsDebounce(UI_CONTROLS_EDITOR_STATE *s);
static UI_CONTROLS_CHOICE M_Listen(UI_CONTROLS_EDITOR_STATE *s);
static UI_CONTROLS_CHOICE M_ListenDebounce(UI_CONTROLS_EDITOR_STATE *s);

static void M_CurrentLayout(const UI_CONTROLS_EDITOR_STATE *s);
static void M_GroupsHeader(const UI_CONTROLS_EDITOR_STATE *s);
static void M_InputChoice(UI_CONTROLS_EDITOR_STATE *s, INPUT_ROLE role);
static void M_InputLabel(const UI_CONTROLS_EDITOR_STATE *s, INPUT_ROLE role);
static void M_FooterButton(
    UI_CONTROLS_EDITOR_STATE *s, INPUT_ROLE role, const char *role_label);
static void M_Group(
    UI_CONTROLS_EDITOR_STATE *s, const UI_CONTROLS_EDITOR_GROUP *group);
static void M_Footer(UI_CONTROLS_EDITOR_STATE *s);

static int32_t M_GetVisibleRows(void)
{
    const int32_t res_h =
        Scaler_CalcInverse(Viewport_GetHeight(), SCALER_TARGET_TEXT);
    if (res_h <= 240) {
        return 5;
    } else if (res_h <= 252) {
        return 6;
    } else if (res_h <= 266) {
        return 7;
    } else if (res_h <= 282) {
        return 8;
    } else if (res_h <= 300) {
        return 9;
    } else if (res_h <= 320) {
        return 10;
    } else if (res_h <= 342) {
        return 11;
    } else if (res_h <= 370) {
        return 12;
    } else if (res_h <= 420) {
        return 13;
    } else if (res_h <= 480) {
        return 15;
    } else {
        return 16;
    }
}

static INPUT_ROLE M_GetInputRole(
    const UI_CONTROLS_EDITOR_GROUP *const group, const int32_t row)
{
    return group->roles[row];
}

static int32_t M_GetInputRoleCount(const UI_CONTROLS_EDITOR_GROUP *const group)
{
    int32_t row = 0;
    while (M_GetInputRole(group, row) != (INPUT_ROLE)-1) {
        row++;
    }
    return row;
}

static void M_CycleLayout(UI_CONTROLS_EDITOR_STATE *const s, const int32_t dir)
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

static void M_CycleGroup(UI_CONTROLS_EDITOR_STATE *const s, const int32_t dir)
{
    if (dir == -1) {
        if (s->active_group == &m_Groups[0]) {
            while (s->active_group[1].header != nullptr) {
                s->active_group++;
            }
        } else {
            s->active_group--;
        }
    } else {
        if (s->active_group[1].header == nullptr) {
            s->active_group = &m_Groups[0];
        } else {
            s->active_group++;
        }
    }
    UI_Scrollable_SetMaxItems(&s->scroll, M_GetInputRoleCount(s->active_group));
}

static void M_ResetLayout(const UI_CONTROLS_EDITOR_STATE *const s)
{
#if TR_VERSION == 1
    Sound_Effect(SFX_MENU_GAMEBOY, nullptr, SPM_NORMAL);
#else
    Sound_Effect(SFX_MENU_SPINOUT, nullptr, SPM_NORMAL);
#endif
    Input_ResetLayout(s->backend, s->active_layout);
    Config_Write();
}

static void M_UnbindKey(const UI_CONTROLS_EDITOR_STATE *const s)
{
#if TR_VERSION == 1
    Sound_Effect(SFX_MENU_GAMEBOY, nullptr, SPM_NORMAL);
#else
    Sound_Effect(SFX_MENU_SPINOUT, nullptr, SPM_NORMAL);
#endif
    Input_UnassignRole(s->backend, s->active_layout, s->active_role);
    Config_Write();
}

static bool M_HandleHoldAction(
    UI_CONTROLS_EDITOR_STATE *const s, const INPUT_ROLE role,
    const M_HOLD_ACTION_FUNC action_func)
{
    if (!Input_IsPressed(s->backend, s->active_layout, role)) {
        return false;
    }
    if (s->hold_timer != -1) {
        s->hold_timer++;
        s->hold_role = role;
        if (s->hold_timer - M_HOLD_TIMER_DEBUFF > M_HOLD_TIMER_MAX) {
            action_func(s);
            s->hold_timer = -1; // Debounce the key
        }
    }
    return true;
}

static void M_CheckResetKeys(UI_CONTROLS_EDITOR_STATE *const s)
{
    bool held = false;
    if (!Input_IsInListenMode() && s->active_layout != INPUT_LAYOUT_DEFAULT) {
        held |= M_HandleHoldAction(s, INPUT_ROLE_RESET_BINDINGS, M_ResetLayout);
        if (s->active_role == (INPUT_ROLE)-1
            || Input_IsRoleUnbindable(s->active_role)) {
            held |= M_HandleHoldAction(s, INPUT_ROLE_UNBIND_KEY, M_UnbindKey);
        }
    }
    if (!held) {
        s->hold_timer = 0;
    }
}

static UI_CONTROLS_CHOICE M_NavigateLayout(UI_CONTROLS_EDITOR_STATE *const s)
{
    M_CheckResetKeys(s);
    if (g_InputDB.menu_confirm) {
        return UI_CONTROLS_CHOICE_EXIT;
    } else if (g_InputDB.menu_back) {
        return UI_CONTROLS_CHOICE_GO_BACK;
    } else if (g_InputDB.menu_left) {
        M_CycleLayout(s, -1);
    } else if (g_InputDB.menu_right) {
        M_CycleLayout(s, 1);
    } else if (g_InputDB.menu_down) {
        s->phase = M_PHASE_NAVIGATE_GROUP;
    } else if (g_InputDB.menu_up && s->active_layout != 0) {
        s->phase = M_PHASE_NAVIGATE_INPUTS;
        UI_Scrollable_SelectLastItem(&s->scroll);
        s->active_role = M_GetInputRole(s->active_group, s->scroll.sel_item);
    } else {
        return UI_CONTROLS_CHOICE_NOOP;
    }
    s->active_role = M_GetInputRole(s->active_group, s->scroll.sel_item);
    return UI_CONTROLS_CHOICE_NOOP;
}

static UI_CONTROLS_CHOICE M_NavigateGroup(UI_CONTROLS_EDITOR_STATE *const s)
{
    M_CheckResetKeys(s);
    if (g_InputDB.menu_confirm) {
        return UI_CONTROLS_CHOICE_EXIT;
    } else if (g_InputDB.menu_back) {
        return UI_CONTROLS_CHOICE_GO_BACK;
    } else if (g_InputDB.menu_left) {
        M_CycleGroup(s, -1);
    } else if (g_InputDB.menu_right) {
        M_CycleGroup(s, 1);
    } else if (g_InputDB.menu_down && s->active_layout != 0) {
        s->phase = M_PHASE_NAVIGATE_INPUTS;
        UI_Scrollable_SelectFirstItem(&s->scroll);
        s->active_role = M_GetInputRole(s->active_group, s->scroll.sel_item);
    } else if (g_InputDB.menu_up) {
        s->phase = M_PHASE_NAVIGATE_LAYOUT;
    }
    return UI_CONTROLS_CHOICE_NOOP;
}

static UI_CONTROLS_CHOICE M_NavigateInputs(UI_CONTROLS_EDITOR_STATE *const s)
{
    M_CheckResetKeys(s);
    if (g_InputDB.menu_confirm) {
        s->phase = M_PHASE_NAVIGATE_INPUTS_DEBOUNCE;
    } else if (g_InputDB.menu_back) {
        return UI_CONTROLS_CHOICE_GO_BACK;
    } else if (g_InputDB.menu_left) {
        M_CycleGroup(s, -1);
    } else if (g_InputDB.menu_right) {
        M_CycleGroup(s, 1);
    } else if (g_InputDB.menu_up) {
        if (!UI_Scrollable_SelectPrev(&s->scroll, false)) {
            s->phase = M_PHASE_NAVIGATE_GROUP;
        }
    } else if (g_InputDB.menu_down) {
        if (!UI_Scrollable_SelectNext(&s->scroll, false)) {
            s->phase = M_PHASE_NAVIGATE_LAYOUT;
        }
    } else {
        return UI_CONTROLS_CHOICE_NOOP;
    }
    s->active_role = M_GetInputRole(s->active_group, s->scroll.sel_item);
    return UI_CONTROLS_CHOICE_NOOP;
}

static UI_CONTROLS_CHOICE M_NavigateInputsDebounce(
    UI_CONTROLS_EDITOR_STATE *const s)
{
    Shell_ProcessEvents();
    Input_Update();
    if (g_Input.any) {
        return UI_CONTROLS_CHOICE_NOOP;
    }
    Input_EnterListenMode();
    s->phase = M_PHASE_LISTEN;
    return UI_CONTROLS_CHOICE_NOOP;
}

static UI_CONTROLS_CHOICE M_Listen(UI_CONTROLS_EDITOR_STATE *const s)
{
    if (!Input_ReadAndAssignRole(
            s->backend, s->active_layout, s->active_role)) {
        return UI_CONTROLS_CHOICE_NOOP;
    }

    Input_ExitListenMode();

    const EVENT event = {
        .name = "key_change",
        .sender = nullptr,
        .data = nullptr,
    };
    EventManager_Fire(s->events, &event);

    s->phase = M_PHASE_LISTEN_DEBOUNCE;
    return UI_CONTROLS_CHOICE_NOOP;
}

static UI_CONTROLS_CHOICE M_ListenDebounce(UI_CONTROLS_EDITOR_STATE *const s)
{
    if (!g_Input.any) {
        s->phase = M_PHASE_NAVIGATE_INPUTS;
    }
    return UI_CONTROLS_CHOICE_NOOP;
}

static void M_CurrentLayout(const UI_CONTROLS_EDITOR_STATE *const s)
{
    UI_BeginAnchor(0.5f, 0.5f);
    if (s->phase == M_PHASE_NAVIGATE_LAYOUT) {
        UI_BeginFrame(UI_FRAME_SELECTED_OPTION);
    }
    UI_BeginPad(2.0f, 1.0f);
    UI_Label(Input_GetLayoutName(s->active_layout));
    UI_EndPad();
    if (s->phase == M_PHASE_NAVIGATE_LAYOUT) {
        UI_EndFrame();
    }
    UI_EndAnchor();
}

static void M_GroupsHeader(const UI_CONTROLS_EDITOR_STATE *const s)
{
    UI_BeginAnchor(0.5f, 0.5f);
    UI_BeginStackEx((UI_STACK_SETTINGS) {
        .orientation = UI_STACK_HORIZONTAL,
        .align = { .h = UI_STACK_H_ALIGN_CENTER },
        .spacing = { .h = 10.0f },
    });
    const UI_CONTROLS_EDITOR_GROUP *group = m_Groups;
    while (group->header != nullptr) {
        UI_BeginAnchor(0.5f, 0.5f);
        if (group == s->active_group) {
            UI_BeginFrame(
                s->phase == M_PHASE_NAVIGATE_GROUP ? UI_FRAME_SELECTED_OPTION
                                                   : UI_FRAME_OUTLINE_ONLY);
        }
        UI_BeginPad(2.0f, 1.0f);
        UI_Label(GameString_Get(group->header));
        UI_EndPad();
        if (group == s->active_group) {
            UI_EndFrame();
        }
        UI_EndAnchor();
        group++;
    }
    UI_EndStack();
    UI_EndAnchor();
}

static void M_InputLabel(
    const UI_CONTROLS_EDITOR_STATE *const s, const INPUT_ROLE role)
{
    const bool is_selected = s->active_role == role
        && (s->phase == M_PHASE_NAVIGATE_INPUTS
            || s->phase == M_PHASE_NAVIGATE_INPUTS_DEBOUNCE);
    if (is_selected) {
        UI_BeginFrame(UI_FRAME_SELECTED_OPTION);
    }
    UI_Label(Input_GetRoleName(role));
    if (is_selected) {
        UI_EndFrame();
    }
}

static void M_InputChoice(
    UI_CONTROLS_EDITOR_STATE *const s, const INPUT_ROLE role)
{
    const bool is_flashing =
        Input_IsKeyConflicted(s->backend, s->active_layout, role);
    const bool is_selected =
        s->active_role == role && s->phase == M_PHASE_LISTEN;

    if (is_flashing) {
        UI_BeginFlash(&s->flash);
    }
    if (is_selected) {
        UI_BeginFrame(UI_FRAME_SELECTED_OPTION);
    }
    UI_Label(Input_GetKeyName(s->backend, s->active_layout, role));
    if (is_selected) {
        UI_EndFrame();
    }
    if (is_flashing) {
        UI_EndFlash();
    }
}

static void M_FooterButton(
    UI_CONTROLS_EDITOR_STATE *const s, const INPUT_ROLE role,
    const char *const role_label)
{
    char tmp_buf[60];
    char button_label[80];
    sprintf(
        tmp_buf, GS(MISC_HOLD_FMT),
        Input_GetKeyName(s->backend, s->active_layout, role));
    sprintf(button_label, "%s: %s", role_label, tmp_buf);

    const float pad[2] = { 6.0f, 3.0f };

    UI_BeginSpan();
    UI_BeginPad(pad[0], pad[1]);
    UI_Label(button_label);
    UI_EndPad();
    if (s->hold_role == role && s->hold_timer >= M_HOLD_TIMER_DEBUFF) {
        UI_Bar((UI_BAR_SETTINGS) {
            .color = TR_VERSION == 2 ? BC_GREEN : BC_GOLD,
            .value = s->hold_timer - M_HOLD_TIMER_DEBUFF,
            .max_value = M_HOLD_TIMER_MAX,
            .w = 0.0, // Span will make it expand anyway!
            .h = 0.0,
        });
    }
    UI_EndSpan();
}

static void M_Group(
    UI_CONTROLS_EDITOR_STATE *const s,
    const UI_CONTROLS_EDITOR_GROUP *const group)
{
    UI_BeginStack(UI_STACK_VERTICAL);
    for (int32_t i = 0; i < s->scroll.vis_items; i++) {
        const int32_t row = s->scroll.first_item + i;
        if (row >= s->scroll.max_items) {
            UI_Spacer(0.0f, TEXT_HEIGHT_FIXED);
        } else {
            const INPUT_ROLE role = group->roles[row];
            UI_BeginStack(UI_STACK_HORIZONTAL);
            UI_BeginResize(s->input_size, -1.0f);
            UI_BeginAnchor(0.0f, 0.5f);
            M_InputChoice(s, role);
            UI_EndAnchor();
            UI_EndResize();
            UI_BeginResize(s->label_size, -1.0f);
            UI_BeginAnchor(0.0f, 0.5f);
            M_InputLabel(s, role);
            UI_EndAnchor();
            UI_EndResize();
            UI_EndStack();
        }
    }
    UI_EndStack();
}

static void M_Footer(UI_CONTROLS_EDITOR_STATE *const s)
{
    UI_BeginStackEx((UI_STACK_SETTINGS) {
        .orientation = UI_STACK_HORIZONTAL,
        .align = { .h = UI_STACK_H_ALIGN_DISTRIBUTE },
        .spacing = { .h = 40.0f },
    });
    UI_BeginHide(
        Input_IsInListenMode() || s->active_layout == INPUT_LAYOUT_DEFAULT);
    M_FooterButton(s, INPUT_ROLE_RESET_BINDINGS, GS(ACTION_RESET_DEFAULTS));
    UI_EndHide();

    UI_BeginHide(
        Input_IsInListenMode() || s->active_layout == INPUT_LAYOUT_DEFAULT
        || s->active_role == (INPUT_ROLE)-1
        || !Input_IsRoleUnbindable(s->active_role));
    M_FooterButton(s, INPUT_ROLE_UNBIND_KEY, GS(ACTION_UNBIND));
    UI_EndHide();
    UI_EndStack();
}

void UI_ControlsEditor_Init(
    UI_CONTROLS_EDITOR_STATE *const s, EVENT_MANAGER *events)
{
    s->events = events;
    s->hold_timer = 0;
    UI_Flash_Init(&s->flash, LOGIC_FPS * 2 / 3);

    s->max_group_items = 0;
    for (const UI_CONTROLS_EDITOR_GROUP *group = m_Groups;
         group->header != nullptr; group++) {
        s->max_group_items =
            MAX(s->max_group_items, M_GetInputRoleCount(group));
    }

    s->active_group = &m_Groups[0];
    s->scroll.first_item = 0;
    s->scroll.sel_item = -1;
    s->scroll.vis_items = MIN(s->max_group_items, M_GetVisibleRows());
    s->scroll.max_items = M_GetInputRoleCount(s->active_group);

    s->label_size = 0.0f;
    for (int32_t i = 0; i < INPUT_ROLE_NUMBER_OF; i++) {
        float w;
        UI_Label_Measure(Input_GetRoleName(i), &w, nullptr);
        s->label_size = MAX(s->label_size, w / g_Config.ui.text_scale);
    }
    s->input_size = 80;
}

void UI_ControlsEditor_Free(UI_CONTROLS_EDITOR_STATE *const s)
{
    UI_Flash_Free(&s->flash);
}

void UI_ControlsEditor_Reinit(
    UI_CONTROLS_EDITOR_STATE *s, INPUT_BACKEND backend, int32_t layout)
{
    s->backend = backend;
    s->active_layout = layout;
    s->scroll.sel_item = 0;
    s->active_role = M_GetInputRole(s->active_group, s->scroll.sel_item);
    s->phase = M_PHASE_NAVIGATE_LAYOUT;
}

UI_CONTROLS_CHOICE UI_ControlsEditor_Control(UI_CONTROLS_EDITOR_STATE *const s)
{
    UI_Flash_Control(&s->flash);
    switch (s->phase) {
    case M_PHASE_NAVIGATE_LAYOUT:
        return M_NavigateLayout(s);
    case M_PHASE_NAVIGATE_GROUP:
        return M_NavigateGroup(s);
    case M_PHASE_NAVIGATE_INPUTS:
        return M_NavigateInputs(s);
    case M_PHASE_NAVIGATE_INPUTS_DEBOUNCE:
        return M_NavigateInputsDebounce(s);
    case M_PHASE_LISTEN:
        return M_Listen(s);
    case M_PHASE_LISTEN_DEBOUNCE:
        return M_ListenDebounce(s);
    default:
        return UI_CONTROLS_CHOICE_NOOP;
    }
}

void UI_ControlsEditor(UI_CONTROLS_EDITOR_STATE *const s)
{
    UI_BeginModal(0.5f, 0.55f);
    UI_BeginStackEx((UI_STACK_SETTINGS) {
        .orientation = UI_STACK_VERTICAL,
        .align = { .h = UI_STACK_H_ALIGN_SPAN },
    });

    UI_BeginWindow();
    UI_WindowTitle(GS(CONTROLS_CUSTOMIZE));
    UI_BeginWindowBody();

    UI_BeginStackEx((UI_STACK_SETTINGS) {
        .orientation = UI_STACK_VERTICAL,
        .align = { .h = UI_STACK_H_ALIGN_SPAN },
    });
    M_CurrentLayout(s);
    M_GroupsHeader(s);
    UI_Spacer(0.0f, 5.0f);

    UI_BeginStack(UI_STACK_HORIZONTAL);
    M_Group(s, s->active_group);
    UI_EndStack();

    UI_EndStack();
    UI_EndWindowBody();
    UI_EndWindow();

    UI_Spacer(0.0f, 5.f);
    M_Footer(s);
    UI_EndStack();
    UI_EndModal();
}
