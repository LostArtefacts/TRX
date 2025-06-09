#include "game/ui/dialogs/settings.h"

#include "config.h"
#include "debug.h"
#include "game/input.h"
#include "game/scaler.h"
#include "game/ui/elements/anchor.h"
#include "game/ui/elements/frame.h"
#include "game/ui/elements/hide.h"
#include "game/ui/elements/label.h"
#include "game/ui/elements/modal.h"
#include "game/ui/elements/pad.h"
#include "game/ui/elements/resize.h"
#include "game/ui/elements/row_arrows.h"
#include "game/ui/elements/scrollable_area.h"
#include "game/ui/elements/spacer.h"
#include "game/ui/elements/stack.h"
#include "game/ui/elements/tab_switch.h"
#include "game/ui/elements/window.h"
#include "game/viewport.h"
#include "memory.h"
#include "strings.h"
#include "utils.h"

#include <math.h>

typedef struct {
    const UI_SETTINGS_ENUM_ENTRY *entry;
    int32_t position;
    int32_t count;
} M_ENUM_LOOKUP;

static int32_t M_GetVisibleRows(void);
static uint8_t *M_GetColorComponent(const UI_SETTINGS_OPTION *option);
static M_ENUM_LOOKUP M_GetEnumEntry(const UI_SETTINGS_OPTION *option);
static const char *M_FormatRowValue(
    const UI_SETTINGS_STATE *s, int32_t row_idx);
static float M_MeasureMaxValueWidth(const UI_SETTINGS_OPTION *option);
static bool M_CanChangeValue(
    const UI_SETTINGS_STATE *s, int32_t row_idx, int32_t dir);
static bool M_RequestChangeValue(
    const UI_SETTINGS_STATE *s, int32_t row_idx, int32_t dir);
static float M_GetMaxLabelWidth(const UI_SETTINGS_STATE *s);
static float M_GetMaxValueWidth(const UI_SETTINGS_STATE *s);
static bool M_CanExamine(const UI_SETTINGS_STATE *s);
static void M_OptionsChanged(UI_SETTINGS_STATE *s);

static int32_t M_GetVisibleRows(void)
{
    const int32_t res_h =
        Scaler_CalcInverse(Viewport_GetHeight(), SCALER_TARGET_TEXT);
    static struct {
        int32_t threshold;
        int32_t rows;
    } thresholds[] = {
        { 240, 5 },  { 252, 6 },  { 266, 7 },  { 282, 8 },
        { 300, 9 },  { 320, 10 }, { 342, 11 }, { 370, 12 },
        { 420, 13 }, { 480, 15 }, { -1, 16 },
    };
    for (int32_t i = 0;; i++) {
        if (res_h <= thresholds[i].threshold || thresholds[i].threshold == -1) {
            return thresholds[i].rows;
        }
    }
}

static uint8_t *M_GetColorComponent(const UI_SETTINGS_OPTION *const option)
{
    RGB_888 *const color = option->target;
    switch ((int32_t)(intptr_t)option->misc) {
    case 0:
        return &color->r;
    case 1:
        return &color->g;
    case 2:
        return &color->b;
    }
    ASSERT_FAIL();
    return nullptr;
}

static M_ENUM_LOOKUP M_GetEnumEntry(const UI_SETTINGS_OPTION *const option)
{
    M_ENUM_LOOKUP result = {
        .entry = nullptr,
        .position = -1,
        .count = 0,
    };
    int32_t current_pos = 0;
    const UI_SETTINGS_ENUM_ENTRY *entry =
        &((UI_SETTINGS_ENUM_ENTRY *)option->misc)[0];
    while (entry->value != -1) {
        if (entry->value == *(int32_t *)option->target) {
            result.entry = entry;
            result.position = current_pos;
        }
        entry++;
        current_pos++;
        result.count++;
    }
    return result;
}

static const char *M_FormatRowValue(
    const UI_SETTINGS_STATE *const s, const int32_t row_idx)
{
    const UI_SETTINGS_OPTION *const option = &s->options[row_idx];
    if (option->custom_handler.format_value != nullptr) {
        return option->custom_handler.format_value(option);
    }
    switch (option->option_type) {
    case COT_BOOL:
        return String_FormatStatic(
            "%s", *(bool *)option->target ? GS(MISC_ON) : GS(MISC_OFF));
    case COT_INVERTED_BOOL:
        return String_FormatStatic(
            "%s", *(bool *)option->target ? GS(MISC_OFF) : GS(MISC_ON));
    case COT_INT32:
        return String_FormatStatic("%d", *(int32_t *)option->target);
    case COT_DOUBLE:
        return String_FormatStatic("%.2f", *(double *)option->target);
    case COT_FLOAT:
        return String_FormatStatic("%.2f", *(float *)option->target);
    case COT_RGB888: {
        const uint8_t *const component = M_GetColorComponent(option);
        return String_FormatStatic("%d", *component);
    }
    case COT_ENUM: {
        const M_ENUM_LOOKUP enum_lookup = M_GetEnumEntry(option);
        ASSERT(enum_lookup.entry != nullptr);
        return (char *)GameString_Get(enum_lookup.entry->name);
    }
    default:
        break;
    }
    return nullptr;
}

static float M_MeasureMaxValueWidth(const UI_SETTINGS_OPTION *const option)
{
    if (option->custom_handler.format_value != nullptr) {
        const char *const value = option->custom_handler.format_value(option);
        const float result = UI_Label_MeasureW(value);
        return result;
    }

    switch (option->option_type) {
    case COT_BOOL:
    case COT_INVERTED_BOOL: {
        const float min_value_w = UI_Label_MeasureW(GS(MISC_OFF));
        const float max_value_w = UI_Label_MeasureW(GS(MISC_ON));
        return MAX(min_value_w, max_value_w);
    }
    case COT_INT32: {
        const char *const min_value_s =
            String_FormatStatic("%d", option->min_value);
        const float min_value_w = UI_Label_MeasureW(min_value_s);
        const char *const max_value_s =
            String_FormatStatic("%d", option->max_value);
        const float max_value_w = UI_Label_MeasureW(max_value_s);
        return MAX(min_value_w, max_value_w);
    }
    case COT_DOUBLE:
    case COT_FLOAT: {
        const char *const min_value_s =
            String_FormatStatic("%.2f", (double)option->min_value / 100.0);
        const float min_value_w = UI_Label_MeasureW(min_value_s);
        const char *const max_value_s =
            String_FormatStatic("%.2f", (double)option->max_value / 100.0);
        const float max_value_w = UI_Label_MeasureW(max_value_s);
        return MAX(min_value_w, max_value_w);
    }
    case COT_RGB888: {
        return UI_Label_MeasureW("255");
    }
    case COT_ENUM: {
        float result = 0.0f;
        const UI_SETTINGS_ENUM_ENTRY *entry = option->misc;
        while (entry->value != -1) {
            const char *const value = GameString_Get(entry->name);
            const float value_w = UI_Label_MeasureW(value);
            result = MAX(result, value_w);
            entry++;
        }
        return result;
    }
    default:
        break;
    }
    return 0.0f;
}

static bool M_CanChangeValue(
    const UI_SETTINGS_STATE *const s, const int32_t row_idx, const int32_t dir)
{
    const UI_SETTINGS_OPTION *const option = &s->options[row_idx];
    if (Config_IsOptionEnforced(option->target)) {
        return false;
    }
    if (option->custom_handler.can_change_value != nullptr) {
        return option->custom_handler.can_change_value(option, dir);
    }
    switch (option->option_type) {
    case COT_BOOL:
    case COT_INVERTED_BOOL:
        return true;
    case COT_INT32:
        if (dir < 0) {
            return *(int32_t *)option->target > option->min_value;
        } else if (dir > 0) {
            return *(int32_t *)option->target < option->max_value;
        }
        break;
    case COT_DOUBLE: {
        const double target_value =
            (round(*(double *)option->target * 100) + dir) / 100.0;
        return target_value >= (double)option->min_value / 100.0
            && target_value <= (double)option->max_value / 100.0;
    }
    case COT_FLOAT: {
        const float target_value =
            (round(*(float *)option->target * 100) + dir) / 100.0f;
        return target_value >= (float)option->min_value / 100.0f
            && target_value <= (float)option->max_value / 100.0f;
    }
    case COT_RGB888: {
        const uint8_t *const component = M_GetColorComponent(option);
        if (dir < 0) {
            return *component > option->min_value;
        } else if (dir > 0) {
            return *component < option->max_value;
        }
        break;
    }
    case COT_ENUM: {
        const M_ENUM_LOOKUP enum_lookup = M_GetEnumEntry(option);
        ASSERT(enum_lookup.entry != nullptr);
        if (dir < 0) {
            return enum_lookup.position > 0;
        } else if (dir > 0) {
            return enum_lookup.position < enum_lookup.count - 1;
        }
        break;
    }
    default:
        break;
    }
    return false;
}

static bool M_RequestChangeValue(
    const UI_SETTINGS_STATE *const s, const int32_t row_idx, const int32_t dir)
{
    if (!M_CanChangeValue(s, row_idx, dir)) {
        return false;
    }

    const UI_SETTINGS_OPTION *const option = &s->options[row_idx];
    if (option->custom_handler.request_change_value != nullptr) {
        if (option->custom_handler.request_change_value(option, dir)) {
            goto changed;
        }
        return false;
    }

    UI_Settings_RequestChange(option, dir);
changed:
    Config_Write();
    return true;
}

static float M_GetMaxLabelWidth(const UI_SETTINGS_STATE *const s)
{
    // Measure the maximum width of the key label to prevent the entire
    // dialog from changing its size as the player navigates the dialog.
    float result = -1.0f;
    if (s->tabs != nullptr) {
        for (int32_t i = 0; i < s->tab_count; i++) {
            for (int32_t j = 0; s->tabs[i].options[j].label_id != nullptr;
                 j++) {
                const UI_SETTINGS_OPTION *const option = &s->tabs[i].options[j];
                const float label_w =
                    UI_Label_MeasureW(GameString_Get(option->label_id));
                result = MAX(label_w, result);
            }
        }
    } else {
        for (int32_t i = 0; s->options[i].label_id != nullptr; i++) {
            const UI_SETTINGS_OPTION *const option = &s->options[i];
            const float label_w =
                UI_Label_MeasureW(GameString_Get(option->label_id));
            result = MAX(label_w, result);
        }
    }
    return result;
}

static float M_GetMaxValueWidth(const UI_SETTINGS_STATE *const s)
{
    // Measure the maximum width of the value label to prevent the entire
    // dialog from changing its size as the player changes the settings.
    float result = -1.0f;

    if (s->tabs != nullptr) {
        for (int32_t i = 0; i < s->tab_count; i++) {
            for (int32_t j = 0; s->tabs[i].options[j].label_id != nullptr;
                 j++) {
                const UI_SETTINGS_OPTION *const option = &s->tabs[i].options[j];
                const float value_w = M_MeasureMaxValueWidth(option);
                result = MAX(value_w, result);
            }
        }
    } else {
        for (int32_t i = 0; s->options[i].label_id != nullptr; i++) {
            const UI_SETTINGS_OPTION *const option = &s->options[i];
            const float value_w = M_MeasureMaxValueWidth(option);
            result = MAX(value_w, result);
        }
    }

    result += UI_Label_MeasureW("\\{button left}");
    result += UI_Label_MeasureW("\\{button right}");
    result += UI_ROW_ARROWS_TIGHT * 2;
    return result;
}

static bool M_CanExamine(const UI_SETTINGS_STATE *const s)
{
    if (s->phase != UI_SETTINGS_PHASE_EDIT_SETTINGS) {
        return false;
    }
    const int32_t sel_row = UI_Scrollable_GetSelectedItem(&s->scroll);
    if (sel_row < 0 || s->options[sel_row].description_id == nullptr) {
        return false;
    }
    const UI_SETTINGS_OPTION *const option = &s->options[sel_row];
    const char *const title = GameString_Get(option->label_id);
    const char *const text = GameString_Get(option->description_id);
    return title != nullptr && text != nullptr;
}

static void M_OptionsChanged(UI_SETTINGS_STATE *const s)
{
    int32_t row_count = 0;
    for (int32_t i = 0; s->options[i].label_id != nullptr; i++) {
        row_count++;
    }
    UI_Scrollable_SetMaxItems(&s->scroll, row_count);
    s->max_label_w = M_GetMaxLabelWidth(s) / g_Config.ui.text_scale;
    s->max_value_w = M_GetMaxValueWidth(s) / g_Config.ui.text_scale;
}

void UI_Settings_Init(
    UI_SETTINGS_STATE *const s, const GAME_STRING_ID title,
    const UI_SETTINGS_OPTION *const options)
{
    s->title = title;
    s->options = options;
    s->max_group_items = 0;
    for (int32_t i = 0; s->options[i].label_id != nullptr; i++) {
        s->max_group_items++;
    }
    s->tab_count = 0;
    s->tabs = nullptr;
    s->tab_switch = nullptr;
    s->phase = UI_SETTINGS_PHASE_EDIT_SETTINGS;
    M_OptionsChanged(s);
}

void UI_Settings_InitWithTabs(
    UI_SETTINGS_STATE *s, const GAME_STRING_ID title, const int32_t tab_count,
    const UI_SETTINGS_TAB *const tabs)
{
    ASSERT(tabs != nullptr);
    s->title = title;
    s->options = tabs[0].options;
    s->tab_count = tab_count;
    s->tabs = tabs;
    s->max_group_items = 0;
    UI_TAB_SWITCH_TAB tab_switch_tabs[tab_count];
    for (int32_t i = 0; i < tab_count; i++) {
        tab_switch_tabs[i].header = GameString_Get(tabs[i].header);
        int32_t tab_items = 0;
        for (int32_t j = 0; tabs[i].options[j].label_id != nullptr; j++) {
            tab_items++;
        }
        s->max_group_items = MAX(tab_items, s->max_group_items);
    }
    s->tab_switch = UI_TabSwitch_Init(tab_count, tab_switch_tabs);
    s->phase = UI_SETTINGS_PHASE_NAVIGATE_TABS;
    M_OptionsChanged(s);
}

void UI_Settings_Free(UI_SETTINGS_STATE *const s)
{
    if (s->tab_switch != nullptr) {
        UI_TabSwitch_Free(s->tab_switch);
        s->tab_switch = nullptr;
    }
    if (s->description.show) {
        UI_TextDialog_Free(&s->description.state);
        s->description.show = false;
    }
}

bool UI_Settings_Control(UI_SETTINGS_STATE *const s)
{
    UI_Scrollable_SetVisibleItems(
        &s->scroll, MIN(s->max_group_items, M_GetVisibleRows()));
    if (s->description.show) {
        UI_TextDialog_Control(&s->description.state);
        if (g_InputDB.menu_back || g_InputDB.look) {
            UI_TextDialog_Free(&s->description.state);
            s->description.show = false;
            return false;
        }
        return false;
    }

    if (s->phase == UI_SETTINGS_PHASE_NAVIGATE_TABS) {
        if (UI_TabSwitch_Control(s->tab_switch)) {
            s->options = s->tabs[s->tab_switch->active_tab_idx].options;
            M_OptionsChanged(s);
            return false;
        } else if (g_InputDB.menu_down) {
            s->phase = UI_SETTINGS_PHASE_EDIT_SETTINGS;
            UI_Scrollable_SelectFirstItem(&s->scroll);
        } else if (g_InputDB.menu_up && g_Config.ui.enable_wraparound) {
            s->phase = UI_SETTINGS_PHASE_EDIT_SETTINGS;
            UI_Scrollable_SelectLastItem(&s->scroll);
        } else if (g_InputDB.menu_back) {
            return true;
        }
    } else if (s->phase == UI_SETTINGS_PHASE_EDIT_SETTINGS) {
        const int32_t sel_row = UI_Scrollable_GetSelectedItem(&s->scroll);
        if (g_InputDB.look && M_CanExamine(s)) {
            const UI_SETTINGS_OPTION *const option = &s->options[sel_row];
            const char *title = GameString_Get(option->label_id);
            const char *text = GameString_Get(option->description_id);
            if (title != nullptr && text != nullptr) {
                if (Config_IsOptionEnforced(option->target)) {
                    title = String_FormatStatic("%s*", title);
                    text = String_FormatStatic(
                        "* %s\n\n%s",
                        GS(COMMON_SETTINGS_FROZEN_OPTION_DISCLAIMER), text);
                }
                UI_TextDialog_Init(
                    &s->description.state, title, text,
                    MIN(UI_GetCanvasWidth() * 2.0 / 3.0f,
                        s->max_label_w + s->max_value_w + 10),
                    (size_t)M_GetVisibleRows(), true);
                s->description.show = true;
            }
            return false;
        }
        if (g_InputDB.menu_up) {
            if (!UI_Scrollable_SelectPrev(&s->scroll, false)) {
                if (s->tab_switch != nullptr) {
                    s->phase = UI_SETTINGS_PHASE_NAVIGATE_TABS;
                } else if (g_Config.ui.enable_wraparound) {
                    UI_Scrollable_SelectLastItem(&s->scroll);
                }
            }
        } else if (g_InputDB.menu_down) {
            if (!UI_Scrollable_SelectNext(&s->scroll, false)
                && g_Config.ui.enable_wraparound) {
                if (s->tab_switch != nullptr) {
                    s->phase = UI_SETTINGS_PHASE_NAVIGATE_TABS;
                } else {
                    UI_Scrollable_SelectFirstItem(&s->scroll);
                }
            }
        } else if (g_InputDB.menu_back) {
            return true;
        } else {
            if (g_InputDB.menu_left && sel_row >= 0) {
                M_RequestChangeValue(s, sel_row, -1);
            } else if (g_InputDB.menu_right && sel_row >= 0) {
                M_RequestChangeValue(s, sel_row, +1);
            }
        }
    }
    return false;
}

void UI_Settings_RequestChange(
    const UI_SETTINGS_OPTION *const option, const int32_t dir)
{
    int32_t delta = g_Input.slow ? option->delta_slow : option->delta_fast;
    if (delta == 0) {
        delta = 1;
    }
    delta *= dir;

    switch (option->option_type) {
    case COT_BOOL:
    case COT_INVERTED_BOOL:
        *(bool *)option->target = !*(bool *)option->target;
        break;
    case COT_INT32:
        *(int32_t *)option->target += delta;
        break;
    case COT_DOUBLE:
        *(double *)option->target =
            (round(*(double *)option->target * 100) + delta) / 100.0f;
        if (*(double *)option->target == -0.0) {
            *(double *)option->target = 0.0;
        }
        break;
    case COT_FLOAT:
        *(float *)option->target =
            (round(*(float *)option->target * 100) + delta) / 100.0f;
        if (*(float *)option->target == -0.0f) {
            *(float *)option->target = 0.0f;
        }
        break;
    case COT_RGB888: {
        uint8_t *const component = M_GetColorComponent(option);
        int32_t component_i = *component;
        component_i += delta;
        CLAMP(component_i, 0, 255);
        *component = component_i;
        break;
    }
    case COT_ENUM: {
        const M_ENUM_LOOKUP enum_lookup = M_GetEnumEntry(option);
        const UI_SETTINGS_ENUM_ENTRY *const next_entry =
            &((UI_SETTINGS_ENUM_ENTRY *)
                  option->misc)[enum_lookup.position + delta];
        *(int32_t *)option->target = next_entry->value;
        break;
    }
    }
}

void UI_Settings(UI_SETTINGS_STATE *const s)
{
    const int32_t sel_row = UI_Scrollable_GetSelectedItem(&s->scroll);
    UI_BeginModal(0.5f, 0.6f);
    UI_BeginStackEx((UI_STACK_SETTINGS) {
        .orientation = UI_STACK_VERTICAL,
        .spacing = { .v = 5.0f },
        .align = { .h = UI_STACK_H_ALIGN_SPAN },
    });

    UI_BeginWindow();
    UI_WindowTitle(GameString_Get(s->title));
    UI_BeginWindowBody();

    UI_BeginStackEx((UI_STACK_SETTINGS) {
        .orientation = UI_STACK_VERTICAL,
        .align = { .h = UI_STACK_H_ALIGN_SPAN },
    });
    if (s->tab_switch != nullptr) {
        UI_TabSwitch(
            s->tab_switch, s->phase == UI_SETTINGS_PHASE_NAVIGATE_TABS);
        UI_Spacer(0.0f, 8.0f);
    }

    UI_BeginScrollableArea(&s->scroll, s->tab_count > 0);

    for (int32_t i = 0; i < s->scroll.vis_items; i++) {
        const int32_t row = s->scroll.first_item + i;
        if (row >= s->scroll.max_items) {
            UI_Spacer(0.0f, UI_TEXT_HEIGHT);
            continue;
        }

        const bool is_row_focused =
            s->phase == UI_SETTINGS_PHASE_EDIT_SETTINGS && row == sel_row;
        if (!UI_Scrollable_IsItemVisible(&s->scroll, row)) {
            UI_BeginResize(-1.0f, 0.0f);
        } else {
            UI_BeginResize(-1.0f, -1.0f);
        }

        UI_BeginPad(
            TR_VERSION == 1 ? -1.0f : 0.0f, TR_VERSION == 1 ? -1.0f : 0.0f);
        if (is_row_focused) {
            UI_BeginFrame(UI_FRAME_SELECTED_OPTION);
        }
        UI_BeginPad(
            s->row_pad + (TR_VERSION == 1 ? 1.0f : 0.0f),
            TR_VERSION == 1 ? 1.0f : 0.0f);
        UI_BeginStackEx((UI_STACK_SETTINGS) {
            .orientation = UI_STACK_HORIZONTAL,
            .align = { .h = UI_STACK_H_ALIGN_DISTRIBUTE },
        });
        UI_BeginResize(s->max_label_w, -1.0f);
        {
            const UI_SETTINGS_OPTION *const option = &s->options[row];
            const char *const name = GameString_Get(option->label_id);
            if (Config_IsOptionEnforced(option->target)) {
                UI_LabelFmt("%s*", name);
            } else {
                UI_Label(name);
            }
        }
        UI_EndResize();
        UI_Spacer(20.0f, 0.0f);

        UI_BeginResize(s->max_value_w, -1.0f);
        UI_BeginAnchor(1.0f, 0.5f);

        UI_BeginRowArrows(
            is_row_focused && M_CanChangeValue(s, row, -1),
            is_row_focused && M_CanChangeValue(s, row, +1),
            UI_ROW_ARROWS_MEDIUM);
        UI_Label(M_FormatRowValue(s, row));
        UI_EndRowArrows();

        UI_EndAnchor();
        UI_EndResize();

        UI_EndStack();

        UI_EndPad();
        if (is_row_focused) {
            UI_EndFrame();
        }
        UI_EndPad();

        UI_EndResize();
    }
    UI_EndScrollableArea(&s->scroll, s->tab_count > 0);
    UI_EndStack();
    UI_EndWindowBody();
    UI_EndWindow();

    // Button hint strip
    UI_BeginHide(!M_CanExamine(s));
    UI_LabelFmt(
        "%s %s",
        Input_GetKeyName(
            INPUT_BACKEND_KEYBOARD, g_Config.input.keyboard_layout,
            INPUT_ROLE_LOOK),
        GS(COMMON_SETTINGS_TOGGLE_HELP));
    UI_EndHide();
    UI_EndStack();

    UI_EndModal();

    if (s->description.show) {
        UI_TextDialog(&s->description.state);
    }
}
