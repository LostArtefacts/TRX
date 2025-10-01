#include "game/ui/dialogs/settings.h"

#include "config.h"
#include "debug.h"
#include "game/const.h"
#include "game/game_string_manager.h"
#include "game/input.h"
#include "game/scaler.h"
#include "game/ui/dialogs/setting_helpers/enums.h"
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

#define M_HOLD_TIMER_DEBUFF (LOGIC_FPS / 3)
#define M_HOLD_TIMER_MAX LOGIC_FPS

typedef struct {
    const UI_SETTINGS_ENUM_ENTRY *entry;
    int32_t position;
    int32_t count;
} M_ENUM_LOOKUP;

typedef struct UI_SETTINGS_STATE {
    UI_SETTINGS_PHASE phase;
    const UI_SETTINGS_OPTION *options;
    UI_SCROLLABLE scroll;

    int32_t max_group_items;
    float max_label_w;
    float max_value_w;

    int32_t tab_count;
    const UI_SETTINGS_TAB *tabs;
    int32_t active_tab_idx;
    UI_TAB_SWITCH_STATE *tab_switch;
    GAME_STRING_ID title;

    struct {
        bool show;
        UI_TEXT_DIALOG_STATE *state;
    } description;

    int32_t listener_id;
} UI_SETTINGS_STATE;

static int32_t M_GetVisibleRows(void)
{
    const int32_t res_h =
        Scaler_CalcInverse(Viewport_GetHeight(VIEWPORT_UI), SCALER_TARGET_TEXT);
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

// Map a visible row index to the corresponding option, skipping hidden ones.
static const UI_SETTINGS_OPTION *M_GetOptionByRow(
    const UI_SETTINGS_STATE *const s, const int32_t row_idx)
{
    int32_t count = 0;
    for (int32_t i = 0; s->options[i].label_id != nullptr; i++) {
        const UI_SETTINGS_OPTION *const opt = &s->options[i];
        if (Config_IsOptionHidden(opt->target)) {
            continue;
        }
        if (count == row_idx) {
            return opt;
        }
        count++;
    }
    return nullptr;
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
    const UI_SETTINGS_OPTION *const option = M_GetOptionByRow(s, row_idx);
    if (option == nullptr) {
        return nullptr;
    }
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
    case COT_FLOAT_PERCENT:
        return String_FormatStatic(
            "%.00f%%", (*(float *)option->target) * 100.0f);
    case COT_STRING:
        return String_FormatStatic("%s", *(char **)option->target);
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
    case COT_FLOAT_PERCENT: {
        const char *const min_value_s =
            String_FormatStatic("%.00f%%", (double)option->min_value);
        const float min_value_w = UI_Label_MeasureW(min_value_s);
        const char *const max_value_s =
            String_FormatStatic("%.00f%%", (double)option->max_value);
        const float max_value_w = UI_Label_MeasureW(max_value_s);
        return MAX(min_value_w, max_value_w);
    }
    case COT_RGB888:
        return UI_Label_MeasureW("255");
    case COT_STRING:
        return UI_Label_MeasureW(*(char **)option->target);
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
    const UI_SETTINGS_OPTION *const option = M_GetOptionByRow(s, row_idx);
    if (option == nullptr || Config_IsOptionEnforced(option->target)) {
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

    case COT_FLOAT:
    case COT_FLOAT_PERCENT: {
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

    case COT_STRING:
        return false;

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

    const UI_SETTINGS_OPTION *const option = M_GetOptionByRow(s, row_idx);
    if (option->custom_handler.request_change_value != nullptr) {
        if (option->custom_handler.request_change_value(option, dir)) {
            goto changed;
        }
        return false;
    }

    UI_Settings_RequestChange(option, dir);
changed:
    Config_Update();
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

static bool M_CanExamine(
    const UI_SETTINGS_STATE *const s, const int32_t row_idx)
{
    if (s->phase != UI_SETTINGS_PHASE_EDIT_SETTINGS || row_idx < 0) {
        return false;
    }
    const UI_SETTINGS_OPTION *const option = M_GetOptionByRow(s, row_idx);
    if (option == nullptr || option->description_id == 0) {
        return false;
    }
    const char *const title = GameString_Get(option->label_id);
    const char *const text = GameString_Get(option->description_id);
    return title != nullptr && text != nullptr;
}

// Returns whether the selected setting may be reset to its default.
static bool M_CanRestoreDefault(
    const UI_SETTINGS_STATE *const s, const int32_t row_idx)
{
    if (s->phase != UI_SETTINGS_PHASE_EDIT_SETTINGS || row_idx < 0) {
        return false;
    }
    const UI_SETTINGS_OPTION *const option = M_GetOptionByRow(s, row_idx);
    if (option == nullptr || option->target == nullptr
        || Config_IsOptionEnforced(option->target)) {
        return false;
    }
    return !Config_IsOptionAtDefault(option->target);
}

// Reset the selected setting back to its compiled-in default value.
static void M_RestoreDefault(
    const UI_SETTINGS_STATE *const s, const int32_t row_idx)
{
    const UI_SETTINGS_OPTION *const option = M_GetOptionByRow(s, row_idx);
    if (option != nullptr) {
        Config_RestoreOptionDefault(option->target);
        Config_Update();
    }
}

static void M_RecomputeSizes(UI_SETTINGS_STATE *const s)
{
    int32_t row_count = 0;
    for (int32_t i = 0; s->options[i].label_id != nullptr; i++) {
        if (!Config_IsOptionHidden(s->options[i].target)) {
            row_count++;
        }
    }
    UI_Scrollable_SetMaxItems(&s->scroll, row_count);
    UI_Scrollable_SetVisibleItems(
        &s->scroll, MIN(s->max_group_items, M_GetVisibleRows()));
    s->max_label_w = M_GetMaxLabelWidth(s) / g_Config.ui.text_scale;
    s->max_value_w = M_GetMaxValueWidth(s) / g_Config.ui.text_scale;
}

// Helpers for label/value rendering: add a star if enforced,
// and wrap entire string in {dim}…{/dim} if the option is grayed-out.
static void M_OptionLabel(
    const UI_SETTINGS_OPTION *const option, const char *const text,
    const bool star_if_enforced)
{
    const bool is_available = option == nullptr
        || option->custom_handler.is_available == nullptr
        || option->custom_handler.is_available(option);
    const bool is_enforced = star_if_enforced && option != nullptr
        && Config_IsOptionEnforced(option->target);
    const char *const suffix = is_enforced ? "*" : "";

    if (!is_available) {
        UI_LabelFmt("\\{dim}%s%s\\{/dim}", text, suffix);
    } else if (is_enforced) {
        UI_LabelFmt("%s%s", text, suffix);
    } else {
        UI_Label(text);
    }
}

static void M_Footer(const UI_SETTINGS_STATE *const s)
{
    const int32_t row_idx = UI_Scrollable_GetSelectedItem(&s->scroll);
    UI_BeginStackEx((UI_STACK_SETTINGS) {
        .orientation = UI_STACK_HORIZONTAL,
        .align = { .h = UI_STACK_H_ALIGN_DISTRIBUTE },
        .spacing = { .h = 20 },
    });
    UI_BeginHide(!M_CanExamine(s, row_idx));
    UI_LabelFmt("\\{input look} %s", GS(COMMON_SETTINGS_TOGGLE_HELP));
    UI_EndHide();
    UI_BeginHide(!M_CanRestoreDefault(s, row_idx));
    UI_LabelFmt("\\{input unbind_key} %s", GS(COMMON_SETTINGS_RESTORE_DEFAULT));
    UI_EndHide();
    UI_EndStack();
}

static void M_HandleLanguageReload(const EVENT *const event, void *const data)
{
    UI_SETTINGS_STATE *const s = data;
    M_RecomputeSizes(s);
}

static UI_SETTINGS_STATE *M_InitCommon(const GAME_STRING_ID title)
{
    UI_SETTINGS_STATE *const s = Memory_Alloc(sizeof(*s));
    s->title = title;
    s->listener_id =
        GameStringManager_SubscribeReload(M_HandleLanguageReload, s);
    return s;
}

UI_SETTINGS_STATE *UI_Settings_Init(
    const GAME_STRING_ID title, const UI_SETTINGS_OPTION *const options)
{
    UI_SETTINGS_STATE *const s = M_InitCommon(title);
    s->options = options;
    s->max_group_items = 0;
    for (int32_t i = 0; s->options[i].label_id != nullptr; i++) {
        if (!Config_IsOptionHidden(s->options[i].target)) {
            s->max_group_items++;
        }
    }
    s->tab_count = 0;
    s->tabs = nullptr;
    s->tab_switch = nullptr;
    s->phase = UI_SETTINGS_PHASE_EDIT_SETTINGS;
    M_RecomputeSizes(s);
    return s;
}

UI_SETTINGS_STATE *UI_Settings_InitWithTabs(
    const GAME_STRING_ID title, const int32_t tab_count,
    const UI_SETTINGS_TAB *const tabs)
{
    ASSERT(tabs != nullptr);
    UI_SETTINGS_STATE *const s = M_InitCommon(title);

    // Filter tabs to only those with visible (non-hidden) options.
    int32_t visible_tab_count = 0;
    for (int32_t i = 0; i < tab_count; i++) {
        int32_t tab_items = 0;
        for (int32_t j = 0; tabs[i].options[j].label_id != nullptr; j++) {
            if (!Config_IsOptionHidden(tabs[i].options[j].target)) {
                tab_items++;
            }
        }
        if (tab_items > 0) {
            visible_tab_count++;
        }
    }

    // Gather visible tabs.
    UI_TAB_SWITCH_TAB tab_switch_tabs[tab_count];
    UI_SETTINGS_TAB *visible_tabs = nullptr;
    if (visible_tab_count > 0) {
        visible_tabs =
            Memory_Alloc(sizeof(UI_SETTINGS_TAB) * visible_tab_count);
    }
    int32_t vt = 0;
    s->max_group_items = 0;
    for (int32_t i = 0; i < tab_count; i++) {
        int32_t tab_items = 0;
        for (int32_t j = 0; tabs[i].options[j].label_id != nullptr; j++) {
            if (!Config_IsOptionHidden(tabs[i].options[j].target)) {
                tab_items++;
            }
        }
        if (tab_items <= 0) {
            continue;
        }
        visible_tabs[vt] = tabs[i];
        tab_switch_tabs[vt].header.one_off = nullptr;
        tab_switch_tabs[vt].header.live_ptr =
            GameString_GetPtr(tabs[i].header_gs);
        s->max_group_items = MAX(tab_items, s->max_group_items);
        vt++;
    }

    // Always use visible_tabs (may be zero-length if every tab hidden).
    s->tabs = visible_tabs;
    s->tab_count = visible_tab_count;
    s->options =
        (visible_tab_count > 0) ? visible_tabs[0].options : tabs[0].options;
    s->tab_switch = UI_TabSwitch_Init(s->tab_count, tab_switch_tabs);

    s->phase = UI_SETTINGS_PHASE_NAVIGATE_TABS;
    M_RecomputeSizes(s);
    return s;
}

void UI_Settings_Free(UI_SETTINGS_STATE *const s)
{
    if (s->listener_id >= 0) {
        GameStringManager_UnsubscribeReload(s->listener_id);
        s->listener_id = -1;
    }
    if (s->tab_switch != nullptr) {
        UI_TabSwitch_Free(s->tab_switch);
        s->tab_switch = nullptr;
    }
    Memory_FreePointer(&s->tabs);
    if (s->description.show) {
        UI_TextDialog_Free(s->description.state);
        s->description.state = nullptr;
        s->description.show = false;
    }
    Memory_Free(s);
}

bool UI_Settings_Control(UI_SETTINGS_STATE *const s)
{
    UI_Scrollable_SetVisibleItems(
        &s->scroll, MIN(s->max_group_items, M_GetVisibleRows()));
    if (s->description.show) {
        UI_TextDialog_Control(s->description.state);
        if (g_InputDB.menu_back || g_InputDB.look) {
            UI_TextDialog_Free(s->description.state);
            s->description.state = nullptr;
            s->description.show = false;
            return false;
        }
        return false;
    }

    if (s->phase == UI_SETTINGS_PHASE_NAVIGATE_TABS) {
        if (UI_TabSwitch_Control(s->tab_switch, UI_TAB_SWITCH_NORMAL)) {
            s->options = s->tabs[s->tab_switch->active_tab_idx].options;
            UI_Scrollable_SelectFirstItem(&s->scroll);
            M_RecomputeSizes(s);
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
        if (g_InputDB.menu_left && sel_row >= 0) {
            M_RequestChangeValue(s, sel_row, -1);
        } else if (g_InputDB.menu_right && sel_row >= 0) {
            M_RequestChangeValue(s, sel_row, +1);
        } else if (
            s->tab_switch != nullptr && !g_Input.menu_left
            && !g_Input.menu_right
            && UI_TabSwitch_Control(s->tab_switch, UI_TAB_SWITCH_NO_ARROWS)) {
            s->options = s->tabs[s->tab_switch->active_tab_idx].options;
            M_RecomputeSizes(s);
            return false;
        } else if (g_InputDB.look && M_CanExamine(s, sel_row)) {
            s->description.show = true;
            s->description.state = UI_TextDialog_Init(
                MIN(UI_GetCanvasWidth() * 2.0 / 3.0f,
                    s->max_label_w + s->max_value_w + 10),
                (size_t)M_GetVisibleRows(), true);
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
        } else if (
            g_InputDB.unbind_key && sel_row >= 0
            && M_CanRestoreDefault(s, sel_row)) {
            M_RestoreDefault(s, sel_row);
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
    case COT_FLOAT_PERCENT:
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
    case COT_STRING:
        // It doesn't make sense to cycle strings like this
        break;
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
    if (s->tab_switch != nullptr && s->tab_count > 0) {
        UI_TabSwitch(
            s->tab_switch, s->phase == UI_SETTINGS_PHASE_NAVIGATE_TABS);
        UI_Spacer(0.0f, 8.0f);
    }

    UI_BeginScrollableArea(&s->scroll, s->tab_count > 0);

    if (s->scroll.vis_items == 0) {
        UI_BeginResize(-1.0f, -1.0f);
        UI_BeginPad(
            TR_VERSION == 1 ? -1.0f : 0.0f, TR_VERSION == 1 ? -1.0f : 0.0f);
        UI_BeginStackEx((UI_STACK_SETTINGS) {
            .orientation = UI_STACK_VERTICAL,
            .align = { .h = UI_STACK_H_ALIGN_CENTER },
        });
        UI_Label(GS(COMMON_SETTINGS_ALL_HIDDEN_DISCLAIMER));
        UI_EndStack();
        UI_EndPad();
        UI_EndResize();
    }

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
            (TR_VERSION == 1 ? 1.0f : 0.0f), TR_VERSION == 1 ? 1.0f : 0.0f);
        UI_BeginStackEx((UI_STACK_SETTINGS) {
            .orientation = UI_STACK_HORIZONTAL,
            .align = { .h = UI_STACK_H_ALIGN_DISTRIBUTE },
        });
        UI_BeginResize(s->max_label_w, -1.0f);
        {
            const UI_SETTINGS_OPTION *const option = M_GetOptionByRow(s, row);
            const char *const name =
                option ? GameString_Get(option->label_id) : "";
            M_OptionLabel(option, name, /* star_if_enforced */ true);
        }
        UI_EndResize();
        UI_Spacer(20.0f, 0.0f);

        UI_BeginResize(s->max_value_w, -1.0f);
        UI_BeginAnchor(1.0f, 0.5f);

        UI_BeginRowArrows(
            is_row_focused && M_CanChangeValue(s, row, -1),
            is_row_focused && M_CanChangeValue(s, row, +1),
            UI_ROW_ARROWS_MEDIUM);
        {
            const UI_SETTINGS_OPTION *const option = M_GetOptionByRow(s, row);
            const char *const value = M_FormatRowValue(s, row);
            M_OptionLabel(option, value, /* star_if_enforced */ false);
        }
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
    M_Footer(s);

    UI_EndStack();
    UI_EndModal();

    if (s->description.show) {
        const int32_t sel_row = UI_Scrollable_GetSelectedItem(&s->scroll);
        const UI_SETTINGS_OPTION *const option = M_GetOptionByRow(s, sel_row);
        if (option != nullptr) {
            const char *title = GameString_Get(option->label_id);
            const char *text = GameString_Get(option->description_id);
            if (title != nullptr && text != nullptr) {
                if (Config_IsOptionEnforced(option->target)) {
                    title = String_FormatStatic("%s*", title);
                    text = String_FormatStatic(
                        "* %s\n\n%s",
                        *GS_PTR(COMMON_SETTINGS_FROZEN_OPTION_DISCLAIMER),
                        text);
                }
                UI_TextDialog(s->description.state, title, text);
            }
        }
    }
}
