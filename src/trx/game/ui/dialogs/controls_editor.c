#include <trx/game/ui/dialogs/controls_editor.h>

#include <trx/config.h>
#include <trx/config/section.h>
#include <trx/core/strings.h>
#include <trx/core/utils.h>
#include <trx/game/const.h>
#include <trx/game/game_strings/entries.h>
#include <trx/game/gun.h>
#include <trx/game/gun/registry.h>
#include <trx/game/input.h>
#include <trx/game/sound.h>
#include <trx/game/ui/elements/anchor.h>
#include <trx/game/ui/elements/frame.h>
#include <trx/game/ui/elements/hide.h>
#include <trx/game/ui/elements/label.h>
#include <trx/game/ui/elements/modal.h>
#include <trx/game/ui/elements/pad.h>
#include <trx/game/ui/elements/requester.h>
#include <trx/game/ui/elements/resize.h>
#include <trx/game/ui/elements/row_arrows.h>
#include <trx/game/ui/elements/spacer.h>
#include <trx/game/ui/elements/span.h>
#include <trx/game/ui/elements/stack.h>
#include <trx/game/ui/elements/window.h>
#include <trx/game/ui/scaler.h>
#include <trx/game/ui/touch_overlay.h>
#include <trx/game/viewport.h>
#include <trx/version.h>

#define M_MIN_INPUT_SIZE 80.0f
#define M_INPUT_PADDING 6.0f
#define M_HEADER_ROWS 2
#define M_HEADER_SPACING 9.0f
#define M_FOOTER_ROWS 1
#define M_FOOTER_SPACING 5.0f
#define M_BUTTON_SPACING 40.0f
#define M_MEASURE_SLACK 1.05f
#define M_MIN_VISIBLE_ROWS 3
#define M_MAX_VISIBLE_ROWS 15

typedef enum {
    M_PHASE_NAVIGATE_LAYOUT,
    M_PHASE_NAVIGATE_GROUP,
    M_PHASE_NAVIGATE_INPUTS,
    M_PHASE_NAVIGATE_INPUTS_DEBOUNCE,
    M_PHASE_LISTEN,
    M_PHASE_LISTEN_DEBOUNCE,
    M_PHASE_EXIT,
} M_PHASE;

static const UI_CONTROLS_EDITOR_GROUP m_Groups[] = {
    {
        .header_gs = GS_ID("general/settings/controls/tabs/basics"),
        .rows =
            (UI_CONTROLS_EDITOR_ROW[]) {
                { .role = INPUT_ROLE_UP },
                { .role = INPUT_ROLE_DOWN },
                { .role = INPUT_ROLE_LEFT },
                { .role = INPUT_ROLE_RIGHT },
                { .role = INPUT_ROLE_JUMP },
                { .role = INPUT_ROLE_STEP_LEFT },
                { .role = INPUT_ROLE_STEP_RIGHT },
                { .role = INPUT_ROLE_ROLL },
                { .role = INPUT_ROLE_SLOW },
                { .role = INPUT_ROLE_SPRINT },
                { .role = INPUT_ROLE_CROUCH },
                { .role = INPUT_ROLE_ACTION },
                { .role = INPUT_ROLE_DRAW_WEAPON },
                { .role = INPUT_ROLE_LOOK },
                { .role = (INPUT_ROLE)-1 },
            },
    },

    {
        .header_gs = GS_ID("general/settings/controls/tabs/items"),
        .rows =
            (UI_CONTROLS_EDITOR_ROW[]) {
                { .role = INPUT_ROLE_USE_FLARE },
                { .role = INPUT_ROLE_USE_SMALL_MEDI },
                { .role = INPUT_ROLE_USE_BIG_MEDI },
                { .role = INPUT_ROLE_EQUIP_PISTOLS },
                { .role = INPUT_ROLE_EQUIP_SHOTGUN },
                { .role = INPUT_ROLE_EQUIP_MAGNUMS },
                { .role = INPUT_ROLE_EQUIP_AUTOS },
                { .role = INPUT_ROLE_EQUIP_DESERT_EAGLE },
                { .role = INPUT_ROLE_EQUIP_UZIS },
                { .role = INPUT_ROLE_EQUIP_HARPOON },
                { .role = INPUT_ROLE_EQUIP_M16 },
                { .role = INPUT_ROLE_EQUIP_MP5 },
                { .role = INPUT_ROLE_EQUIP_ROCKET_LAUNCHER },
                { .role = INPUT_ROLE_EQUIP_GRENADE_LAUNCHER },
                { .role = (INPUT_ROLE)-1 },
            },
    },

    {
        .header_gs = GS_ID("general/settings/controls/tabs/misc"),
        .rows =
            (UI_CONTROLS_EDITOR_ROW[]) {
                { .role = INPUT_ROLE_CHANGE_TARGET },
                { .role = INPUT_ROLE_CAMERA_UP },
                { .role = INPUT_ROLE_CAMERA_DOWN },
                { .role = INPUT_ROLE_CAMERA_LEFT },
                { .role = INPUT_ROLE_CAMERA_RIGHT },
                { .role = INPUT_ROLE_CAMERA_FORWARD },
                { .role = INPUT_ROLE_CAMERA_BACK },
                { .role = INPUT_ROLE_USE_BINOCULARS },
                { .role = INPUT_ROLE_CHANGE_OUTFIT },
                { .role = INPUT_ROLE_FLY_CHEAT },
                { .role = INPUT_ROLE_ITEM_CHEAT },
                { .role = INPUT_ROLE_LEVEL_SKIP_CHEAT },
                { .role = INPUT_ROLE_TURBO_CHEAT },
                { .role = INPUT_ROLE_FAST_FORWARD_CHEAT },
                { .role = INPUT_ROLE_SLOW_MOTION_CHEAT },
                { .role = (INPUT_ROLE)-1 },
            },
    },

    {
        .header_gs = GS_ID("general/settings/controls/tabs/system"),
        .rows =
            (UI_CONTROLS_EDITOR_ROW[]) {
                { .role = INPUT_ROLE_INVENTORY },
                { .role = INPUT_ROLE_SAVE },
                { .role = INPUT_ROLE_LOAD },
                { .role = INPUT_ROLE_QUICK_SAVE },
                { .role = INPUT_ROLE_QUICK_LOAD },
                { .role = INPUT_ROLE_PAUSE },
                // { .role = INPUT_ROLE_SCREENSHOT }, // handled specially
                { .role = INPUT_ROLE_FPS },
                { .role = INPUT_ROLE_TOGGLE_FULLSCREEN },
                { .role = INPUT_ROLE_ENTER_CONSOLE },
                { .role = INPUT_ROLE_TOGGLE_PHOTO_MODE },
                { .role = INPUT_ROLE_TOGGLE_UI },
                { .role = INPUT_ROLE_TOGGLE_BILINEAR_FILTER },
                { .role = INPUT_ROLE_TOGGLE_TRAPEZOID_FILTER },
                { .role = INPUT_ROLE_SWITCH_UPSCALING },
                { .role = INPUT_ROLE_SWITCH_BORDERS },
                { .role = INPUT_ROLE_TOGGLE_WIREFRAME },
                { .role = INPUT_ROLE_TOGGLE_TEXTURES },
                { .role = INPUT_ROLE_CYCLE_LIGHTING_MODEL },
                { .role = (INPUT_ROLE)-1 },
            },
    },

    {
        .header_gs = nullptr,
        .rows = nullptr,
    },
};

static bool M_IsTouch(const UI_CONTROLS_EDITOR_STATE *const s)
{
    return s->backend == INPUT_BACKEND_TOUCH;
}

static int32_t M_GetInputRoleCount(const UI_CONTROLS_EDITOR_GROUP *const group)
{
    int32_t count = 0;
    for (int32_t i = 0; group->rows[i].role != (INPUT_ROLE)-1; i++) {
        count++;
    }
    return count;
}

static float M_GetRowHeight(void)
{
    float height = 0.0f;
    UI_Label_Measure("0", nullptr, &height);
    return height / UI_Scaler_GetTextScale();
}

static int32_t M_GetVisibleRows(
    const UI_CONTROLS_EDITOR_STATE *const s, const float fit_scale)
{
    const float row_height = M_GetRowHeight();
    const UI_WINDOW_SETTINGS window = {
        .title = GS("general/settings/controls/customize"),
        .scrollable = &s->scroll,
        .title_spacing = -1.0f,
        .reserve_scroll_space = true,
    };
    const float around = UI_Window_GetChromeHeight(&window)
        + M_HEADER_ROWS * row_height + M_HEADER_SPACING
        + M_FOOTER_ROWS * row_height + M_FOOTER_SPACING;

    const float available = UI_GetSafeCanvasHeight()
        / (UI_Scaler_GetTextScale() * MAX(0.01f, fit_scale) * M_MEASURE_SLACK);
    int32_t rows = (available - around) / row_height;
    CLAMP(rows, M_MIN_VISIBLE_ROWS, M_MAX_VISIBLE_ROWS);
    return rows;
}

static bool M_IsRoleUsable(const INPUT_ROLE role)
{
    switch (role) {
    case INPUT_ROLE_USE_FLARE:
        return Gun_Registry_Get(LGT_FLARE)->is_available;
    case INPUT_ROLE_EQUIP_MAGNUMS:
        return Gun_Registry_Get(LGT_MAGNUMS)->is_available;
    case INPUT_ROLE_EQUIP_AUTOS:
        return Gun_Registry_Get(LGT_AUTOS)->is_available;
    case INPUT_ROLE_EQUIP_DESERT_EAGLE:
        return Gun_Registry_Get(LGT_DESERT_EAGLE)->is_available;
    case INPUT_ROLE_EQUIP_HARPOON:
        return Gun_Registry_Get(LGT_HARPOON)->is_available;
    case INPUT_ROLE_EQUIP_M16:
        return Gun_Registry_Get(LGT_M16)->is_available;
    case INPUT_ROLE_EQUIP_MP5:
        return Gun_Registry_Get(LGT_MP5)->is_available;
    case INPUT_ROLE_EQUIP_GRENADE_LAUNCHER:
        return Gun_Registry_Get(LGT_GRENADE)->is_available;
    case INPUT_ROLE_EQUIP_ROCKET_LAUNCHER:
        return Gun_Registry_Get(LGT_ROCKET)->is_available;
    case INPUT_ROLE_FLY_CHEAT:
    case INPUT_ROLE_ITEM_CHEAT:
    case INPUT_ROLE_LEVEL_SKIP_CHEAT:
    case INPUT_ROLE_TURBO_CHEAT:
    case INPUT_ROLE_FAST_FORWARD_CHEAT:
    case INPUT_ROLE_SLOW_MOTION_CHEAT:
        return g_Config.gameplay.enable_cheats;
    case INPUT_ROLE_USE_BINOCULARS:
        return g_Config.gameplay.enable_binoculars;
    default:
        break;
    }
    return true;
}

// The key column takes its width from the longest binding any layout holds, so
// that a combo such as Alt+Return does not run into the column beside it.
static void M_MeasureColumns(UI_CONTROLS_EDITOR_STATE *const s)
{
    s->label_size = 0.0f;
    s->input_size = M_MIN_INPUT_SIZE;
    s->widest_role = nullptr;
    for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
        float w;
        UI_Label_Measure(Input_GetRoleName(role), &w, nullptr);
        w /= UI_Scaler_GetTextScale();
        if (w > s->label_size) {
            s->label_size = w;
            s->widest_role = Input_GetRoleName(role);
        }

        for (INPUT_LAYOUT layout = 0; layout < INPUT_LAYOUT_NUMBER_OF;
             layout++) {
            for (int32_t slot = 0; slot < INPUT_BINDING_SLOTS; slot++) {
                const char *const key_name =
                    Input_GetKeyName(s->backend, layout, role, slot);
                if (key_name == nullptr) {
                    continue;
                }
                UI_Label_Measure(key_name, &w, nullptr);
                s->input_size =
                    MAX(s->input_size,
                        w / UI_Scaler_GetTextScale() + M_INPUT_PADDING);
            }
        }
    }
}

static void M_ResetLayout(void *const arg)
{
    UI_CONTROLS_EDITOR_STATE *const s = arg;
    Sound_Effect(
        g_TRVersion == 1 ? SFX_MENU_GAMEBOY : SFX_MENU_SPINOUT, nullptr,
        SPM_NORMAL);
    Input_ResetLayout(s->backend, s->active_layout);
    Config_SectionChanged();
    Config_Update();
}

static void M_UnbindKey(void *const arg)
{
    UI_CONTROLS_EDITOR_STATE *const s = arg;
    Sound_Effect(
        g_TRVersion == 1 ? SFX_MENU_GAMEBOY : SFX_MENU_SPINOUT, nullptr,
        SPM_NORMAL);
    Input_UnassignRole(
        s->backend, s->active_layout, s->active_role, s->active_slot);
    Config_SectionChanged();
    Config_Update();
}

static bool M_CanResetLayout(const UI_CONTROLS_EDITOR_STATE *const s)
{
    return !Input_IsInListenMode()
        && s->phase != M_PHASE_NAVIGATE_INPUTS_DEBOUNCE
        && s->active_layout != INPUT_LAYOUT_DEFAULT;
}

static bool M_CanUnbindKey(const UI_CONTROLS_EDITOR_STATE *const s)
{
    return !Input_IsInListenMode()
        && (s->phase == M_PHASE_NAVIGATE_INPUTS
            || s->phase == M_PHASE_LISTEN_DEBOUNCE)
        && s->active_layout != INPUT_LAYOUT_DEFAULT
        && s->active_role != (INPUT_ROLE)-1
        && Input_IsRoleUnbindable(s->active_role);
}

static void M_CheckResetKeys(UI_CONTROLS_EDITOR_STATE *const s)
{
    if (M_CanResetLayout(s)) {
        UI_ProgressButton_Control(s->reset_bindings_button);
    }
    if (M_CanUnbindKey(s)) {
        UI_ProgressButton_Control(s->unbind_key_button);
    }
}

static UI_CONTROLS_CHOICE M_NavigateLayout(UI_CONTROLS_EDITOR_STATE *const s)
{
    M_CheckResetKeys(s);
    if (g_InputDB.menu_confirm) {
        return UI_CONTROLS_CHOICE_EXIT;
    } else if (g_InputDB.menu_back) {
        return UI_CONTROLS_CHOICE_GO_BACK;
    } else if (
        UI_TabSwitch_Control(s->layout_tab_switch, UI_TAB_SWITCH_NORMAL)) {
        s->active_layout = s->layout_tab_switch->active_tab_idx;
        const EVENT event = {
            .name = "layout_change",
            .sender = nullptr,
            .data = nullptr,
        };
        EventManager_Fire(s->events, &event);
    } else if (g_InputDB.menu_down) {
        s->phase = M_PHASE_NAVIGATE_GROUP;
    } else if (
        g_InputDB.menu_up && s->active_layout != 0
        && g_Config.ui.enable_wraparound) {
        s->phase = M_PHASE_NAVIGATE_INPUTS;
        UI_Scrollable_SelectLastItem(&s->scroll);
        s->active_role = s->active_group->rows[s->scroll.sel_item].role;
    } else {
        return UI_CONTROLS_CHOICE_NOOP;
    }
    s->active_role = s->active_group->rows[s->scroll.sel_item].role;
    return UI_CONTROLS_CHOICE_NOOP;
}

static UI_CONTROLS_CHOICE M_NavigateGroup(UI_CONTROLS_EDITOR_STATE *const s)
{
    M_CheckResetKeys(s);
    if (g_InputDB.menu_confirm) {
        return UI_CONTROLS_CHOICE_EXIT;
    } else if (g_InputDB.menu_back) {
        return UI_CONTROLS_CHOICE_GO_BACK;
    } else if (
        UI_TabSwitch_Control(s->controls_tab_switch, UI_TAB_SWITCH_NORMAL)) {
        s->active_group = &m_Groups[s->controls_tab_switch->active_tab_idx];
        UI_Scrollable_SetMaxItems(
            &s->scroll, M_GetInputRoleCount(s->active_group));
    } else if (g_InputDB.menu_down && s->active_layout != 0) {
        s->phase = M_PHASE_NAVIGATE_INPUTS;
        UI_Scrollable_SelectFirstItem(&s->scroll);
        s->active_role = s->active_group->rows[s->scroll.sel_item].role;
    } else if (g_InputDB.menu_up) {
        s->phase = M_PHASE_NAVIGATE_LAYOUT;
    }
    s->active_role = s->active_group->rows[s->scroll.sel_item].role;
    return UI_CONTROLS_CHOICE_NOOP;
}

static UI_CONTROLS_CHOICE M_NavigateInputs(UI_CONTROLS_EDITOR_STATE *const s)
{
    M_CheckResetKeys(s);
    if (g_InputDB.menu_confirm) {
        s->phase = M_PHASE_NAVIGATE_INPUTS_DEBOUNCE;
    } else if (g_InputDB.menu_back) {
        return UI_CONTROLS_CHOICE_GO_BACK;
    } else if (
        !g_Input.menu_left && !g_Input.menu_right
        && UI_TabSwitch_Control(
            s->controls_tab_switch, UI_TAB_SWITCH_NO_ARROWS)) {
        s->active_group = &m_Groups[s->controls_tab_switch->active_tab_idx];
        UI_Scrollable_SetMaxItems(
            &s->scroll, M_GetInputRoleCount(s->active_group));
    } else if (g_InputDB.menu_left) {
        s->active_slot =
            (s->active_slot - 1 + INPUT_BINDING_SLOTS) % INPUT_BINDING_SLOTS;
    } else if (g_InputDB.menu_right) {
        s->active_slot = (s->active_slot + 1) % INPUT_BINDING_SLOTS;
    } else if (g_InputDB.menu_up) {
        if (!UI_Scrollable_SelectPrev(&s->scroll, false)) {
            s->phase = M_PHASE_NAVIGATE_GROUP;
        }
    } else if (g_InputDB.menu_down) {
        if (!UI_Scrollable_SelectNext(&s->scroll, false)
            && g_Config.ui.enable_wraparound) {
            s->phase = M_PHASE_NAVIGATE_LAYOUT;
        }
    } else {
        return UI_CONTROLS_CHOICE_NOOP;
    }
    s->active_role = s->active_group->rows[s->scroll.sel_item].role;
    return UI_CONTROLS_CHOICE_NOOP;
}

static UI_CONTROLS_CHOICE M_NavigateInputsDebounce(
    UI_CONTROLS_EDITOR_STATE *const s)
{
    Input_Update();
    if (InputState_IsAnyPressed(g_Input)) {
        return UI_CONTROLS_CHOICE_NOOP;
    }
    if (M_IsTouch(s)) {
        TouchOverlay_EnterSelectionMode();
    }
    Input_EnterListenMode();
    s->phase = M_PHASE_LISTEN;
    return UI_CONTROLS_CHOICE_NOOP;
}

static UI_CONTROLS_CHOICE M_Listen(UI_CONTROLS_EDITOR_STATE *const s)
{
    if (!Input_ReadAndAssignRole(
            s->backend, s->active_layout, s->active_role, s->active_slot)) {
        return UI_CONTROLS_CHOICE_NOOP;
    }

    if (M_IsTouch(s)) {
        TouchOverlay_ExitSelectionMode();
    }
    Input_ExitListenMode();
    M_MeasureColumns(s);

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
    if (!InputState_IsAnyPressed(g_Input)) {
        s->phase = M_PHASE_NAVIGATE_INPUTS;
    }
    return UI_CONTROLS_CHOICE_NOOP;
}

static void M_CurrentLayout(const UI_CONTROLS_EDITOR_STATE *const s)
{
    UI_TabSwitchSingle(
        s->layout_tab_switch,
        s->phase == M_PHASE_NAVIGATE_LAYOUT ? UI_TAB_SWITCH_DRAW_FOCUSED
                                            : UI_TAB_SWITCH_DRAW_IDLE);
}

static void M_GroupsHeader(const UI_CONTROLS_EDITOR_STATE *const s)
{
    UI_TAB_SWITCH_DRAW_MODE mode = UI_TAB_SWITCH_DRAW_IDLE;
    if (s->phase == M_PHASE_NAVIGATE_GROUP) {
        mode = UI_TAB_SWITCH_DRAW_FOCUSED;
    } else if (
        s->phase == M_PHASE_NAVIGATE_INPUTS
        || s->phase == M_PHASE_LISTEN_DEBOUNCE) {
        mode = UI_TAB_SWITCH_DRAW_ARROWS;
    }
    if (s->collapsed_groups) {
        UI_TabSwitchSingle(s->controls_tab_switch, mode);
    } else {
        UI_TabSwitch(s->controls_tab_switch, mode);
    }
}

static float M_MeasureWidestTab(UI_TAB_SWITCH_STATE *const tabs)
{
    const int32_t active = tabs->active_tab_idx;
    float result = 0.0f;
    for (int32_t i = 0; i < tabs->tab_count; i++) {
        tabs->active_tab_idx = i;
        UI_BeginMeasure();
        UI_TabSwitchSingle(tabs, UI_TAB_SWITCH_DRAW_ARROWS);
        float width = 0.0f;
        UI_EndMeasure(&width, nullptr);
        result = MAX(result, width);
    }
    tabs->active_tab_idx = active;
    return result;
}

static void M_InputLabel(
    const UI_CONTROLS_EDITOR_STATE *const s,
    const UI_CONTROLS_EDITOR_ROW *const row)
{
    const bool is_selected = s->active_role == row->role
        && (s->phase == M_PHASE_NAVIGATE_INPUTS
            || s->phase == M_PHASE_LISTEN_DEBOUNCE);
    if (is_selected) {
        UI_BeginFrame(UI_FRAME_SELECTED_OPTION);
    }
    const char *const role_name = Input_GetRoleName(row->role);
    if (!M_IsRoleUsable(row->role)) {
        UI_LabelFmt("\\{dim}%s\\{/dim}", role_name);
    } else {
        UI_Label(role_name);
    }
    if (is_selected) {
        UI_EndFrame();
    }
}

static void M_InputSlot(
    UI_CONTROLS_EDITOR_STATE *const s, const UI_CONTROLS_EDITOR_ROW *const row,
    const int32_t slot)
{
    const bool is_flashing =
        Input_IsKeyConflicted(s->backend, s->active_layout, row->role);
    const bool is_active_row = s->active_role == row->role;
    const bool is_listening = is_active_row && s->active_slot == slot
        && (s->phase == M_PHASE_LISTEN
            || s->phase == M_PHASE_NAVIGATE_INPUTS_DEBOUNCE);
    const bool is_slot_selected = is_active_row && s->active_slot == slot
        && (s->phase == M_PHASE_NAVIGATE_INPUTS
            || s->phase == M_PHASE_LISTEN_DEBOUNCE);

    if (is_flashing) {
        UI_BeginFlash(&s->flash);
    }
    if (is_listening || is_slot_selected) {
        UI_BeginFrame(UI_FRAME_SELECTED_OPTION);
    }
    const char *key_name =
        Input_GetKeyName(s->backend, s->active_layout, row->role, slot);
    if (key_name == nullptr) {
        if (s->active_layout != INPUT_LAYOUT_DEFAULT) {
            UI_Label("—");
        } else {
            UI_Label("");
        }
    } else if (!M_IsRoleUsable(row->role)) {
        UI_LabelFmt("\\{dim}%s\\{/dim}", key_name);
    } else {
        UI_Label(key_name);
    }
    if (is_listening || is_slot_selected) {
        UI_EndFrame();
    }
    if (is_flashing) {
        UI_EndFlash();
    }
}

static void M_Group(
    UI_CONTROLS_EDITOR_STATE *const s,
    const UI_CONTROLS_EDITOR_GROUP *const group)
{
    UI_BeginStack(UI_STACK_VERTICAL);
    for (int32_t i = 0; i < s->scroll.vis_items; i++) {
        const int32_t row_idx = s->scroll.first_item + i;
        if (row_idx >= s->scroll.max_items) {
            UI_Spacer(0.0f, UI_TEXT_HEIGHT);
            continue;
        }

        const UI_CONTROLS_EDITOR_ROW *const row = &group->rows[row_idx];
        UI_BeginStack(UI_STACK_HORIZONTAL);
        UI_SetNodeName(UI_NODE_NAME_DIALOG_ROW);
        for (int32_t slot = 0; slot < INPUT_BINDING_SLOTS; slot++) {
            UI_BeginResize(s->input_size, -1.0f);
            UI_BeginAnchor(0.0f, 0.5f);
            M_InputSlot(s, row, slot);
            UI_EndAnchor();
            UI_EndResize();
        }
        UI_BeginResize(s->label_size, -1.0f);
        UI_SetNodeName(UI_NODE_NAME_ROW_TITLE);
        UI_BeginAnchor(0.0f, 0.5f);
        M_InputLabel(s, row);
        UI_EndAnchor();
        UI_EndResize();
        UI_EndStack();
    }
    UI_EndStack();
}

static void M_Footer(UI_CONTROLS_EDITOR_STATE *const s)
{
    UI_BeginStackEx((UI_STACK_SETTINGS) {
        .orientation = UI_STACK_HORIZONTAL,
        .align = { .h = UI_STACK_H_ALIGN_DISTRIBUTE },
        .spacing = { .h = M_BUTTON_SPACING },
    });
    UI_SetNodeName(UI_NODE_NAME_DIALOG_FOOTER);
    UI_BeginHide(!M_CanResetLayout(s));
    UI_ProgressButton(s->reset_bindings_button);
    UI_EndHide();

    UI_BeginHide(!M_CanUnbindKey(s));
    UI_ProgressButton(s->unbind_key_button);
    UI_EndHide();
    UI_EndStack();
}

static float M_GetContentWidth(UI_CONTROLS_EDITOR_STATE *const s)
{
    const float scale = UI_Scaler_GetTextScale();
    const float rows = INPUT_BINDING_SLOTS * s->input_size + s->label_size;

    UI_BeginMeasure();
    M_Footer(s);
    float footer = 0.0f;
    UI_EndMeasure(&footer, nullptr);

    const float header = M_MeasureWidestTab(s->layout_tab_switch);

    UI_BeginMeasure();
    UI_TabSwitch(s->controls_tab_switch, UI_TAB_SWITCH_DRAW_ARROWS);
    float groups = 0.0f;
    UI_EndMeasure(&groups, nullptr);

    const float rest = MAX(MAX(rows, footer / scale), header / scale);
    const float available =
        (UI_GetSafeCanvasWidth() / scale - UI_Window_GetChromeWidth())
        / M_MEASURE_SLACK;
    s->collapsed_groups = groups / scale > MAX(rest, available);
    if (s->collapsed_groups) {
        groups = M_MeasureWidestTab(s->controls_tab_switch);
    }

    UI_Measure_Note("dialog row", s->widest_role, rows);
    UI_Measure_Note(
        "dialog footer", GS("general/actions/reset_defaults"), footer / scale);
    UI_Measure_Note(
        "dialog nav bar", GameString_Get(s->active_group->header_gs),
        groups / scale);

    return MAX(rest, groups / scale) * M_MEASURE_SLACK
        + UI_Window_GetChromeWidth();
}

static void M_Header(void *const user_data)
{
    UI_CONTROLS_EDITOR_STATE *const s = user_data;
    UI_BeginStackEx((UI_STACK_SETTINGS) {
        .orientation = UI_STACK_VERTICAL,
        .align = { .h = UI_STACK_H_ALIGN_SPAN },
        .spacing = { .v = 4.0f },
    });
    M_CurrentLayout(s);
    M_GroupsHeader(s);
    UI_EndStack();
    UI_Spacer(0.0f, 5.0f);
}

void UI_ControlsEditor_Init(
    UI_CONTROLS_EDITOR_STATE *const s, const INPUT_BACKEND backend,
    const int32_t layout, EVENT_MANAGER *const events)
{
    s->backend = backend;
    s->active_layout = layout;
    s->active_slot = 0;
    s->phase = M_PHASE_NAVIGATE_LAYOUT;

    s->events = events;
    UI_Flash_Init(&s->flash, LOGIC_FPS * 2 / 3);

    s->reset_bindings_button = UI_ProgressButton_Init(
        s->backend, INPUT_ROLE_RESET_BINDINGS,
        GS_ID("general/actions/reset_defaults"), M_ResetLayout, s);
    s->unbind_key_button = UI_ProgressButton_Init(
        s->backend, INPUT_ROLE_UNBIND_KEY, GS_ID("general/actions/unbind"),
        M_UnbindKey, s);

    {
        UI_TAB_SWITCH_TAB layout_tabs[INPUT_LAYOUT_NUMBER_OF];
        for (INPUT_LAYOUT i = 0; i < INPUT_LAYOUT_NUMBER_OF; i++) {
            layout_tabs[i].header.one_off = nullptr;
            layout_tabs[i].header.live_ptr = Input_GetLayoutNamePtr(i);
        }
        s->layout_tab_switch =
            UI_TabSwitch_Init(INPUT_LAYOUT_NUMBER_OF, layout_tabs);
        s->layout_tab_switch->active_tab_idx = s->active_layout;
    }

    {
        int32_t tab_count = 0;
        for (int32_t i = 0; m_Groups[i].rows != nullptr; i++) {
            tab_count++;
        }
        UI_TAB_SWITCH_TAB controls_tabs[tab_count];
        for (int32_t i = 0; m_Groups[i].rows != nullptr; i++) {
            controls_tabs[i].header.one_off = nullptr;
            controls_tabs[i].header.live_ptr =
                GameString_GetPtr(m_Groups[i].header_gs);
        }
        s->controls_tab_switch = UI_TabSwitch_Init(tab_count, controls_tabs);
    }

    s->active_group = &m_Groups[0];
    s->scroll.first_item = 0;
    s->scroll.sel_item = 0;
    s->scroll.vis_items =
        M_GetVisibleRows(s, UI_GetFitScale(M_GetContentWidth(s), -1.0f));
    s->scroll.max_items = M_GetInputRoleCount(s->active_group);
    s->active_role = s->active_group->rows[s->scroll.sel_item].role;

    M_MeasureColumns(s);
}

void UI_ControlsEditor_Free(UI_CONTROLS_EDITOR_STATE *const s)
{
    UI_Flash_Free(&s->flash);
    UI_TabSwitch_Free(s->layout_tab_switch);
    s->layout_tab_switch = nullptr;
    UI_TabSwitch_Free(s->controls_tab_switch);
    s->controls_tab_switch = nullptr;
    UI_ProgressButton_Free(s->reset_bindings_button);
    s->reset_bindings_button = nullptr;
    UI_ProgressButton_Free(s->unbind_key_button);
    s->unbind_key_button = nullptr;
}

UI_CONTROLS_CHOICE UI_ControlsEditor_Control(UI_CONTROLS_EDITOR_STATE *const s)
{
    s->scroll.vis_items =
        M_GetVisibleRows(s, UI_GetFitScale(M_GetContentWidth(s), -1.0f));
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
    const float fit_scale = UI_GetFitScale(M_GetContentWidth(s), -1.0f);
    s->scroll.vis_items = M_GetVisibleRows(s, fit_scale);
    UI_Scaler_PushTextScale(fit_scale);
    UI_BeginModal(0.5f, 0.55f);
    UI_BeginResize(M_GetContentWidth(s), -1.0f);
    UI_BeginStackEx((UI_STACK_SETTINGS) {
        .orientation = UI_STACK_VERTICAL,
        .align = { .h = UI_STACK_H_ALIGN_SPAN },
    });

    UI_BeginWindow((UI_WINDOW_SETTINGS) {
        .title = GS("general/settings/controls/customize"),
        .scrollable = &s->scroll,
        .title_spacing = -1.0f,
        .header_func = M_Header,
        .user_data = s,
        .reserve_scroll_space = true,
    });

    UI_BeginStack(UI_STACK_HORIZONTAL);
    M_Group(s, s->active_group);
    UI_EndStack();

    UI_EndWindow();

    UI_Spacer(0.0f, 5.0f);
    M_Footer(s);
    UI_EndStack();
    UI_EndResize();
    UI_EndModal();
    UI_Scaler_PopTextScale();
}
