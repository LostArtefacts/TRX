#include <trx/game/ui/dialogs/settings_editor.h>

#include <trx/config.h>
#include <trx/config/registry.h>
#include <trx/core/dynamic_enum.h>
#include <trx/core/enum_map.h>
#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/core/utils.h>
#include <trx/core/vector.h>
#include <trx/debug.h>
#include <trx/game/input.h>
#include <trx/game/ui.h>
#include <trx/game/ui/dialogs/color_editor.h>
#include <trx/game/ui/dialogs/text.h>
#include <trx/game/ui/scaler.h>
#include <trx/version.h>

#include <math.h>

#define M_BAR_WIDTH 60
#define M_BAR_HEIGHT 12

typedef struct {
    int32_t position;
    int32_t count;
} M_ENUM_LOOKUP;

typedef struct UI_SETTINGS_EDITOR_STATE {
    CONFIG_TAB tab;
    int32_t visible_rows;
    // The rows the tab shows, in the order they are drawn: the tab's own less
    // the ones it hides. Holds `const UI_SETTINGS_ROW *`, so a row index is a
    // position on the screen.
    //
    // Whether a row is hidden is a question about the settings - what another
    // one is at, which of them the game flow took away - so it is settled when
    // the tab is opened and whenever a setting moves, rather than asked again
    // for every row above every row.
    VECTOR *rows;
    // What the rows were arranged against: the options and the handlers, which
    // are made anew when the game changes and announce nothing when they are.
    int32_t config_generation;
    int32_t handler_generation;
    int32_t change_listener;
    UI_SCROLLABLE scroll;
    struct {
        bool show;
        UI_TEXT_DIALOG_STATE *state;
    } description;
    UI_COLOR_EDITOR_DIALOG_STATE *color_editor;
} UI_SETTINGS_EDITOR_STATE;

// What the option accepts, in the units a row steps in: whole numbers for an
// integer setting, hundredths for a float one. False where the option takes
// anything its storage can hold.
static bool M_GetBounds(
    const UI_SETTINGS_ROW *const row, double *const min, double *const max)
{
    if (row->option->bounds == nullptr) {
        return false;
    }
    const TRX_VALUE_TYPE type = row->option->value.type;
    const double scale = type == TVT_FLOAT || type == TVT_DOUBLE ? 100.0 : 1.0;
    *min = row->option->bounds->min * scale;
    *max = row->option->bounds->max * scale;
    return true;
}

// Whether the row may be changed at all. A held setting is not the player's to
// change, whoever is holding it.
static bool M_IsOptionHeld(const UI_SETTINGS_ROW *const row)
{
    return row != nullptr && Config_Option_IsHeld(row->option);
}

static const char *M_GetOptionDescription(const UI_SETTINGS_ROW *const row)
{
    if (row == nullptr) {
        return nullptr;
    }
    return Config_Option_GetDescription(row->option);
}

static const char *M_GetOptionTitle(const UI_SETTINGS_ROW *const row)
{
    if (row == nullptr) {
        return "";
    }
    const char *const result = Config_Option_GetTitle(row->option);
    return result != nullptr ? result : "";
}

// How many values an enum row offers, in the order it cycles them. A handler
// may name that order; otherwise it is the order the enum was defined in.
static int32_t M_GetEnumValueCount(const UI_SETTINGS_ROW *const row)
{
    const int32_t *const order = row->handler->enum_order;
    if (order == nullptr) {
        return EnumMap_GetValueCount(row->option->enum_map);
    }
    int32_t count = 0;
    while (order[count] != -1) {
        count++;
    }
    return count;
}

// The value at a position in that order, or -1 where there is none.
static int32_t M_GetEnumValueAt(
    const UI_SETTINGS_ROW *const row, const int32_t index)
{
    if (index < 0 || index >= M_GetEnumValueCount(row)) {
        return -1;
    }
    const int32_t *const order = row->handler->enum_order;
    if (order == nullptr) {
        return EnumMap_GetValueAt(row->option->enum_map, index);
    }
    return order[index];
}

static bool M_IsEnumValueAvailable(
    const UI_SETTINGS_ROW *const row, const int32_t value)
{
    if (value == -1) {
        return false;
    }
    const UI_SETTING_HANDLER *const handler = row->handler;
    if (handler->is_enum_value_available == nullptr) {
        return true;
    }
    return handler->is_enum_value_available(
        row->option, value, handler->user_data);
}

static UI_BAR_TYPE M_GetBarType(const UI_SETTINGS_ROW *const row)
{
    // The PC and PS1 colours of one bar are two options, and both dress the
    // same bar. Named by where g_Config keeps them, so a setting that moves or
    // goes away is a compile error rather than a bar that stops previewing.
    static const struct {
        const void *mirror;
        UI_BAR_TYPE type;
    } m_BarMirrors[] = {
        { &g_Config.ui.lara_health_bar.color, UI_BAR_LARA_HP },
        { &g_Config.ui.lara_health_bar.color_ps1, UI_BAR_LARA_HP },
        { &g_Config.ui.lara_health_bar.poison_color, UI_BAR_LARA_HP_POISON },
        { &g_Config.ui.lara_health_bar.poison_color_ps1,
          UI_BAR_LARA_HP_POISON },
        { &g_Config.ui.lara_air_bar.color, UI_BAR_LARA_AIR },
        { &g_Config.ui.lara_air_bar.color_ps1, UI_BAR_LARA_AIR },
        { &g_Config.ui.lara_sprint_bar.color, UI_BAR_LARA_STAMINA },
        { &g_Config.ui.lara_sprint_bar.color_ps1, UI_BAR_LARA_STAMINA },
        { &g_Config.ui.lara_exposure_bar.color, UI_BAR_LARA_EXPOSURE },
        { &g_Config.ui.lara_exposure_bar.color_ps1, UI_BAR_LARA_EXPOSURE },
        { &g_Config.ui.enemy_health_bar.color, UI_BAR_ENEMY_HP },
        { &g_Config.ui.enemy_health_bar.color_ps1, UI_BAR_ENEMY_HP },
        { &g_Config.ui.enemy_health_bar.color_allies, UI_BAR_ALLY_HP },
        { &g_Config.ui.enemy_health_bar.color_allies_ps1, UI_BAR_ALLY_HP },
    };

    if (row == nullptr) {
        return (UI_BAR_TYPE)-1;
    }
    for (size_t i = 0; i < ARRAY_SIZE(m_BarMirrors); i++) {
        if (row->option->mirror == m_BarMirrors[i].mirror) {
            return m_BarMirrors[i].type;
        }
    }
    return (UI_BAR_TYPE)-1;
}

static bool M_IsBarColorEnum(const UI_SETTINGS_ROW *const row)
{
    return M_GetBarType(row) != (UI_BAR_TYPE)-1;
}

static bool M_IsColorEditorOption(const UI_SETTINGS_ROW *const row)
{
    return row != nullptr && row->option->value.type == TVT_RGB_888;
}

static bool M_HasAvailableEnumValue(const UI_SETTINGS_ROW *const row)
{
    const int32_t count = M_GetEnumValueCount(row);
    for (int32_t i = 0; i < count; i++) {
        if (M_IsEnumValueAvailable(row, M_GetEnumValueAt(row, i))) {
            return true;
        }
    }
    return false;
}

static bool M_IsOptionHidden(const UI_SETTINGS_ROW *const row)
{
    const UI_SETTING_HANDLER *const handler = row->handler;
    if (handler->is_visible != nullptr
        && !handler->is_visible(row->option, handler->user_data)) {
        return true;
    }
    if (Config_Option_IsHidden(row->option)) {
        return true;
    }
    if (row->option->value.type == TVT_ENUM && !M_HasAvailableEnumValue(row)) {
        return true;
    }
    return false;
}

static void M_ArrangeRows(UI_SETTINGS_EDITOR_STATE *const s)
{
    s->config_generation = Config_GetGeneration();
    s->handler_generation = UI_Settings_GetHandlerGeneration();
    if (s->rows == nullptr) {
        s->rows = Vector_Create(sizeof(const UI_SETTINGS_ROW *));
    }
    Vector_Clear(s->rows);
    const int32_t row_count = UI_Settings_GetRowCount(s->tab);
    for (int32_t i = 0; i < row_count; i++) {
        const UI_SETTINGS_ROW *const row = UI_Settings_GetRow(s->tab, i);
        if (!M_IsOptionHidden(row)) {
            Vector_Add(s->rows, &row);
        }
    }
}

static void M_EnsureRows(UI_SETTINGS_EDITOR_STATE *const s)
{
    if (s->config_generation != Config_GetGeneration()
        || s->handler_generation != UI_Settings_GetHandlerGeneration()) {
        M_ArrangeRows(s);
    }
}

static void M_HandleConfigChange(
    const EVENT *const event, void *const user_data)
{
    M_ArrangeRows(user_data);
}

static const UI_SETTINGS_ROW *M_GetOptionByRow(
    UI_SETTINGS_EDITOR_STATE *const s, const int32_t row_idx)
{
    M_EnsureRows(s);
    if (row_idx < 0 || row_idx >= s->rows->count) {
        return nullptr;
    }
    return *(const UI_SETTINGS_ROW **)Vector_Get(s->rows, row_idx);
}

static int32_t M_GetRowCount(UI_SETTINGS_EDITOR_STATE *const s)
{
    M_EnsureRows(s);
    return s->rows->count;
}

static M_ENUM_LOOKUP M_GetEnumEntry(const UI_SETTINGS_ROW *const row)
{
    M_ENUM_LOOKUP result = {
        .position = -1,
        .count = 0,
    };
    result.count = M_GetEnumValueCount(row);
    for (int32_t i = 0; i < result.count; i++) {
        if (M_GetEnumValueAt(row, i) == row->option->value.as_int) {
            result.position = i;
        }
    }
    return result;
}

static int32_t M_FindNextAvailableEnumPosition(
    const UI_SETTINGS_ROW *const row, const M_ENUM_LOOKUP *const enum_lookup,
    const int32_t dir)
{
    if (enum_lookup->position < 0 || enum_lookup->count <= 0 || dir == 0) {
        return -1;
    }
    const int32_t step = dir < 0 ? -1 : 1;

    for (int32_t pos = enum_lookup->position + step;
         pos >= 0 && pos < enum_lookup->count; pos += step) {
        if (M_IsEnumValueAvailable(row, M_GetEnumValueAt(row, pos))) {
            return pos;
        }
    }

    return -1;
}

static const char *M_FormatRowValue(
    UI_SETTINGS_EDITOR_STATE *const s, const int32_t row_idx)
{
    const UI_SETTINGS_ROW *const row = M_GetOptionByRow(s, row_idx);
    if (row == nullptr) {
        return nullptr;
    }
    const UI_SETTING_HANDLER *const handler = row->handler;
    if (handler->format_value != nullptr) {
        return handler->format_value(row->option, handler->user_data);
    }
    return Config_Option_GetValueAsString(row->option, true);
}

static float M_MeasureMaxValueWidth(const UI_SETTINGS_ROW *const row)
{
    const UI_SETTING_HANDLER *const handler = row->handler;
    if (handler->format_value != nullptr) {
        return UI_Label_MeasureW(
            handler->format_value(row->option, handler->user_data));
    }

    if (M_IsBarColorEnum(row)) {
        return M_BAR_WIDTH * UI_Scaler_GetTextScale();
    }

    switch (row->option->value.type) {
    case TVT_BOOL: {
        const float min_value_w = UI_Label_MeasureW(GS("general/misc/off"));
        const float max_value_w = UI_Label_MeasureW(GS("general/misc/on"));
        return MAX(min_value_w, max_value_w);
    }

    case TVT_S8:
    case TVT_U8:
    case TVT_S16:
    case TVT_U16:
    case TVT_S32:
    case TVT_U32: {
        double min_value = 0.0;
        double max_value = 0.0;
        M_GetBounds(row, &min_value, &max_value);
        const float min_value_w =
            UI_Label_MeasureW(String_FormatStatic("%d", (int32_t)min_value));
        const float max_value_w =
            UI_Label_MeasureW(String_FormatStatic("%d", (int32_t)max_value));
        return MAX(min_value_w, max_value_w);
    }

    case TVT_DOUBLE:
    case TVT_FLOAT: {
        const bool percent = (row->option->flags & CONFIG_OPTION_PERCENT) != 0;
        const char *const fmt = percent ? "%.00f%%" : "%.2f";
        const double scale = percent ? 1.0 : 100.0;
        double min_value = 0.0;
        double max_value = 0.0;
        M_GetBounds(row, &min_value, &max_value);
        const float min_value_w =
            UI_Label_MeasureW(String_FormatStatic(fmt, min_value / scale));
        const float max_value_w =
            UI_Label_MeasureW(String_FormatStatic(fmt, max_value / scale));
        return MAX(min_value_w, max_value_w);
    }

    case TVT_RGB_888:
        return UI_Label_MeasureW("#FFFFFF") + 8.0f * UI_Scaler_GetTextScale()
            + 32.0f * UI_Scaler_GetTextScale();

    case TVT_STRING:
        return UI_Label_MeasureW(row->option->value.as_str);

    case TVT_DYNAMIC_ENUM: {
        float result = 0.0f;
        const void *const token = Config_Option_GetEnumKey(row->option);
        const int32_t count = DynamicEnum_GetValueCount(token);
        for (int32_t i = 0; i < count; i++) {
            const char *const label = DynamicEnum_GetLabelAt(token, i);
            result = MAX(result, UI_Label_MeasureW(label));
        }
        return result;
    }

    case TVT_ENUM: {
        float result = 0.0f;
        const int32_t current_value = row->option->value.as_int;
        const int32_t count = M_GetEnumValueCount(row);
        for (int32_t i = 0; i < count; i++) {
            const int32_t enum_value = M_GetEnumValueAt(row, i);
            const bool is_current = enum_value == current_value;
            if (!is_current && !M_IsEnumValueAvailable(row, enum_value)) {
                continue;
            }
            const char *const value =
                EnumMap_GetLabel(row->option->enum_map, enum_value);
            ASSERT(value != nullptr);
            const float value_w = UI_Label_MeasureW(value);
            result = MAX(result, value_w);
        }
        return result;
    }

    case TVT_XYZ_16:
    case TVT_XYZ_32:
        // Config options are never vectors.
        break;
    }

    return 0.0f;
}

// A float or double config option is edited in hundredths: the stored value
// times 100 is a whole-number position, and min_value/max_value bound that
// position. Both widths ride in as_num, so one set of helpers serves them.
static double M_ReadDecimal(const UI_SETTINGS_ROW *const row)
{
    return row->option->value.as_num;
}

static void M_WriteDecimal(CONFIG_OPTION *const option, const double value)
{
    // Fold -0.0, which round() can produce, into 0.0.
    const TRX_VALUE v = {
        .type = option->value.type,
        .as_num = value == 0.0 ? 0.0 : value,
    };
    Config_Option_Write(option, &v);
}

// The stored value after stepping its hundredths position by `step`.
static double M_SteppedDecimal(
    const UI_SETTINGS_ROW *const row, const int32_t step)
{
    return (round(M_ReadDecimal(row) * 100) + step) / 100.0;
}

static bool M_CanChangeValue(
    UI_SETTINGS_EDITOR_STATE *const s, const int32_t row_idx, const int32_t dir)
{
    const UI_SETTINGS_ROW *const row = M_GetOptionByRow(s, row_idx);
    if (row == nullptr || M_IsOptionHeld(row)) {
        return false;
    }
    const UI_SETTING_HANDLER *const handler = row->handler;
    if (handler->can_change_value != nullptr) {
        return handler->can_change_value(row->option, dir, handler->user_data);
    }

    switch (row->option->value.type) {
    case TVT_BOOL:
        return true;

    case TVT_S8:
    case TVT_U8:
    case TVT_S16:
    case TVT_U16:
    case TVT_S32:
    case TVT_U32: {
        double min_value = 0.0;
        double max_value = 0.0;
        if (!M_GetBounds(row, &min_value, &max_value)) {
            return true;
        }
        const int64_t value = row->option->value.as_int;
        if (dir < 0) {
            return value > (int64_t)min_value;
        } else if (dir > 0) {
            return value < (int64_t)max_value;
        }
        break;
    }

    case TVT_DOUBLE:
    case TVT_FLOAT: {
        double min_value = 0.0;
        double max_value = 0.0;
        if (!M_GetBounds(row, &min_value, &max_value)) {
            return true;
        }
        const double target_value = M_SteppedDecimal(row, dir);
        return target_value >= min_value / 100.0
            && target_value <= max_value / 100.0;
    }

    case TVT_DYNAMIC_ENUM: {
        return DynamicEnum_CanCycle(
            Config_Option_GetEnumKey(row->option), row->option->value.as_str,
            dir);
    }

    case TVT_ENUM: {
        const M_ENUM_LOOKUP enum_lookup = M_GetEnumEntry(row);
        ASSERT(enum_lookup.position >= 0);
        return M_FindNextAvailableEnumPosition(row, &enum_lookup, dir) >= 0;
    }

    case TVT_RGB_888:
    case TVT_STRING:
    case TVT_XYZ_16:
    case TVT_XYZ_32:
        return false;
    }
    return false;
}

static bool M_RequestChangeValue(
    UI_SETTINGS_EDITOR_STATE *const s, const int32_t row_idx, const int32_t dir)
{
    if (!M_CanChangeValue(s, row_idx, dir)) {
        return false;
    }

    const UI_SETTINGS_ROW *const row = M_GetOptionByRow(s, row_idx);
    const UI_SETTING_HANDLER *const handler = row->handler;
    if (handler->request_change_value != nullptr) {
        if (!handler->request_change_value(
                row->option, dir, handler->user_data)) {
            return false;
        }
    } else {
        UI_SettingsEditor_RequestChange(row->option, dir);
    }
    Config_Update();
    return true;
}

static float M_GetMaxLabelWidth(const UI_SETTINGS_EDITOR_STATE *const s)
{
    float result = -1.0f;
    const int32_t row_count = UI_Settings_GetRowCount(s->tab);
    for (int32_t i = 0; i < row_count; i++) {
        const float label_w =
            UI_Label_MeasureW(M_GetOptionTitle(UI_Settings_GetRow(s->tab, i)));
        result = MAX(label_w, result);
    }
    return result;
}

static float M_GetMaxValueWidth(const UI_SETTINGS_EDITOR_STATE *const s)
{
    float result = -1.0f;
    const int32_t row_count = UI_Settings_GetRowCount(s->tab);
    for (int32_t i = 0; i < row_count; i++) {
        const float value_w =
            M_MeasureMaxValueWidth(UI_Settings_GetRow(s->tab, i));
        result = MAX(value_w, result);
    }

    result += UI_Label_MeasureW("\\{button left}");
    result += UI_Label_MeasureW("\\{button right}");
    result += UI_ROW_ARROWS_TIGHT * 2;
    return result;
}

static bool M_IsValueOnOffer(const UI_SETTINGS_ROW *const row)
{
    if (row == nullptr || row->option->value.type != TVT_DYNAMIC_ENUM) {
        return true;
    }
    return DynamicEnum_IsValueEnabled(
        Config_Option_GetEnumKey(row->option), row->option->value.as_str);
}

static void M_OptionLabel(
    const UI_SETTINGS_ROW *const row, const char *const text,
    const bool is_title)
{
    const UI_SETTING_HANDLER *const handler =
        row != nullptr ? row->handler : nullptr;
    const bool is_held = M_IsOptionHeld(row);
    const bool is_available = !is_held
        && (handler == nullptr || handler->is_available == nullptr
            || handler->is_available(row->option, handler->user_data))
        && (is_title || M_IsValueOnOffer(row));
    const bool is_enforced = is_title && is_held;
    const char *const suffix = is_enforced ? "*" : "";

    if (!is_available) {
        UI_LabelFmt("\\{dim}%s%s\\{/dim}", text, suffix);
    } else if (is_enforced) {
        UI_LabelFmt("%s%s", text, suffix);
    } else {
        UI_Label(text);
    }
}

UI_SETTINGS_EDITOR_STATE *UI_SettingsEditor_Init(const CONFIG_TAB tab)
{
    UI_SETTINGS_EDITOR_STATE *const s = Memory_Alloc(sizeof(*s));
    s->tab = tab;
    s->scroll = (UI_SCROLLABLE) {
        .first_item = 0,
        .sel_item = -1,
        .max_items = 0,
        .vis_items = 0,
    };
    s->color_editor = UI_ColorEditorDialog_Init();
    M_ArrangeRows(s);
    // A setting moving is what takes a row away or gives one back, so the
    // arrangement follows the same report the rest of the game does.
    s->change_listener = Config_SubscribeChanges(M_HandleConfigChange, s);
    return s;
}

void UI_SettingsEditor_Free(UI_SETTINGS_EDITOR_STATE *const s)
{
    Config_UnsubscribeChanges(s->change_listener);
    if (s->description.show) {
        UI_TextDialog_Free(s->description.state);
        s->description.state = nullptr;
        s->description.show = false;
    }
    UI_ColorEditorDialog_Free(s->color_editor);
    s->color_editor = nullptr;
    if (s->rows != nullptr) {
        Vector_Free(s->rows);
        s->rows = nullptr;
    }
    Memory_Free(s);
}

float UI_SettingsEditor_GetContentWidth(const UI_SETTINGS_EDITOR_STATE *const s)
{
    return M_GetMaxLabelWidth(s) + 20.0f * UI_Scaler_GetTextScale()
        + M_GetMaxValueWidth(s);
}

float UI_SettingsEditor_GetContentHeight(
    const UI_SETTINGS_EDITOR_STATE *const s)
{
    if (s == nullptr) {
        return -1.0f;
    }

    const int32_t rows = s->scroll.vis_items;
    if (rows <= 0) {
        return -1.0f;
    }

    return rows * UI_TEXT_HEIGHT;
}

int32_t UI_SettingsEditor_GetItemCount(UI_SETTINGS_EDITOR_STATE *const s)
{
    return M_GetRowCount(s);
}

void UI_SettingsEditor_RequestChange(
    CONFIG_OPTION *const option, const int32_t dir)
{
    // Reached from a handler, which has the option and not the row it is shown
    // on, so the pair the helpers want is made here. One lookup per press.
    const UI_SETTINGS_ROW row_storage = { option,
                                          UI_Settings_GetHandler(option) };
    const UI_SETTINGS_ROW *const row = &row_storage;
    int32_t delta = g_Input.menu_fine_adjust ? row->handler->delta_slow
                                             : row->handler->delta_fast;
    if (delta == 0) {
        delta = 1;
    }
    delta *= dir;

    switch (option->value.type) {
    case TVT_BOOL: {
        const TRX_VALUE v = {
            .type = TVT_BOOL,
            .as_bool = !option->value.as_bool,
        };
        Config_Option_Write(option, &v);
        break;
    }

    case TVT_S8:
    case TVT_U8:
    case TVT_S16:
    case TVT_U16:
    case TVT_S32:
    case TVT_U32: {
        TRX_VALUE v = option->value;
        v.as_int += delta;
        Config_Option_Write(option, &v);
        break;
    }

    case TVT_DOUBLE:
    case TVT_FLOAT:
        M_WriteDecimal(option, M_SteppedDecimal(row, delta));
        break;

    case TVT_ENUM: {
        const M_ENUM_LOOKUP enum_lookup = M_GetEnumEntry(row);
        const int32_t pos =
            M_FindNextAvailableEnumPosition(row, &enum_lookup, delta);
        if (pos < 0) {
            break;
        }
        const TRX_VALUE v = {
            .type = TVT_ENUM,
            .as_int = M_GetEnumValueAt(row, pos),
        };
        Config_Option_Write(option, &v);
        break;
    }

    case TVT_DYNAMIC_ENUM: {
        const void *const token = Config_Option_GetEnumKey(option);
        const char *const next =
            DynamicEnum_GetNext(token, option->value.as_str, delta);
        if (next != nullptr || DynamicEnum_IsValidValue(token, nullptr)) {
            Config_Option_SetFromString(option, next, false);
        }
        break;
    }

    case TVT_RGB_888:
    case TVT_STRING:
    case TVT_XYZ_16:
    case TVT_XYZ_32:
        break;
    }
}

void UI_SettingsEditor_RecomputeSizes(
    UI_SETTINGS_EDITOR_STATE *const s, const float max_content_height)
{
    int32_t visible_rows = 0;
    const int32_t row_count = M_GetRowCount(s);
    if (max_content_height > 0.0f) {
        visible_rows = max_content_height / UI_TEXT_HEIGHT;
    }
    CLAMP(visible_rows, 0, row_count);
    s->visible_rows = visible_rows;
    UI_Scrollable_SetMaxItems(&s->scroll, row_count);
    UI_Scrollable_SetVisibleItems(&s->scroll, visible_rows);
}

UI_SCROLLABLE *UI_SettingsEditor_GetScrollable(
    UI_SETTINGS_EDITOR_STATE *const s)
{
    return &s->scroll;
}

bool UI_SettingsEditor_Control(
    UI_SETTINGS_EDITOR_STATE *const s, UI_SETTINGS_PHASE *const dialog_phase)
{
    if (UI_ColorEditorDialog_IsOpen(s->color_editor)) {
        UI_ColorEditorDialog_Control(s->color_editor);
        return true;
    }
    if (s->description.show) {
        UI_TextDialog_Control(s->description.state);
        if (g_InputDB.menu_back || g_InputDB.menu_show_info) {
            UI_TextDialog_Free(s->description.state);
            s->description.state = nullptr;
            s->description.show = false;
        }
        return true;
    }

    const int32_t sel_row = UI_Scrollable_GetSelectedItem(&s->scroll);

    if (g_InputDB.menu_left && sel_row >= 0) {
        M_RequestChangeValue(s, sel_row, -1);
        return true;
    }
    if (g_InputDB.menu_right && sel_row >= 0) {
        M_RequestChangeValue(s, sel_row, +1);
        return true;
    }

    if (g_InputDB.menu_up) {
        if (!UI_Scrollable_SelectPrev(&s->scroll, false)) {
            *dialog_phase = UI_SETTINGS_PHASE_NAVIGATE_TABS;
        }
        return true;
    }
    if (g_InputDB.menu_down) {
        if (!UI_Scrollable_SelectNext(&s->scroll, false)
            && g_Config.ui.enable_wraparound) {
            *dialog_phase = UI_SETTINGS_PHASE_NAVIGATE_TABS;
        }
        return true;
    }
    if (g_InputDB.menu_confirm && sel_row >= 0) {
        const UI_SETTINGS_ROW *const row = M_GetOptionByRow(s, sel_row);
        if (M_IsColorEditorOption(row) && !M_IsOptionHeld(row)) {
            UI_ColorEditorDialog_Open(s->color_editor, row->option);
            return true;
        }
    }
    if (g_InputDB.menu_show_info && sel_row >= 0) {
        const UI_SETTINGS_ROW *const row = M_GetOptionByRow(s, sel_row);
        const char *const title = M_GetOptionTitle(row);
        const char *const text = M_GetOptionDescription(row);
        if (title != nullptr && text != nullptr) {
            s->description.show = true;
            s->description.state = UI_TextDialog_Init(
                UI_GetCanvasWidth() * 2.0f / 3.0f, (size_t)s->visible_rows,
                true);
            return true;
        }
    }
    if (g_InputDB.unbind_key && sel_row >= 0) {
        const UI_SETTINGS_ROW *const row = M_GetOptionByRow(s, sel_row);
        if (row != nullptr && !M_IsOptionHeld(row)
            && !Config_Option_IsAtDefault(row->option)) {
            Config_Option_RestoreDefault(row->option, false);
            Config_Update();
            return true;
        }
    }

    return false;
}

void UI_SettingsEditor_DrawOverlay(UI_SETTINGS_EDITOR_STATE *const s)
{
    UI_ColorEditorDialog(s->color_editor);

    if (s->description.show) {
        const int32_t row_idx = UI_Scrollable_GetSelectedItem(&s->scroll);
        const UI_SETTINGS_ROW *const row = M_GetOptionByRow(s, row_idx);
        if (row != nullptr) {
            const char *title = M_GetOptionTitle(row);
            const char *text = M_GetOptionDescription(row);
            if (title != nullptr && text != nullptr) {
                if (M_IsOptionHeld(row)) {
                    title = String_FormatStatic("%s*", title);
                    text = String_FormatStatic(
                        "* %s\n\n%s",
                        *GS_PTR(
                            "general/settings/common/frozen_option_disclaimer"),
                        text);
                }
                UI_TextDialog(s->description.state, title, text);
            }
        }
    }
}

void UI_SettingsEditor_Draw(
    UI_SETTINGS_EDITOR_STATE *const s, const UI_SCROLLABLE *const dialog_scroll,
    const UI_SETTINGS_PHASE dialog_phase, const float row_width)
{
    const float max_label_w = M_GetMaxLabelWidth(s) / UI_Scaler_GetTextScale();
    const float max_value_w = M_GetMaxValueWidth(s) / UI_Scaler_GetTextScale();
    float label_w = max_label_w;
    const float total_w = max_label_w + 20.0f + max_value_w;
    if (row_width > total_w) {
        label_w += row_width - total_w;
    }

    const int32_t sel_row = UI_Scrollable_GetSelectedItem(dialog_scroll);

    if (dialog_scroll->vis_items == 0) {
        return;
    }

    UI_BeginStack(UI_STACK_VERTICAL);
    for (int32_t i = 0; i < dialog_scroll->vis_items; i++) {
        const int32_t row_idx = dialog_scroll->first_item + i;
        if (row_idx >= dialog_scroll->max_items) {
            UI_Spacer(0.0f, UI_TEXT_HEIGHT);
            continue;
        }

        const bool is_row_focused =
            dialog_phase == UI_SETTINGS_PHASE_EDIT_SETTINGS
            && row_idx == sel_row;
        if (!UI_Scrollable_IsItemVisible(dialog_scroll, row_idx)) {
            UI_BeginResize(-1.0f, 0.0f);
        } else {
            UI_BeginResize(-1.0f, -1.0f);
        }

        UI_BeginPad(
            g_TRVersion == 1 ? -1.0f : 0.0f, g_TRVersion == 1 ? -1.0f : 0.0f);
        if (is_row_focused) {
            UI_BeginFrame(UI_FRAME_SELECTED_OPTION);
        }
        UI_BeginPad(
            (g_TRVersion == 1 ? 1.0f : 0.0f), g_TRVersion == 1 ? 1.0f : 0.0f);
        UI_BeginStackEx((UI_STACK_SETTINGS) {
            .orientation = UI_STACK_HORIZONTAL,
            .align = { .h = UI_STACK_H_ALIGN_DISTRIBUTE },
        });
        UI_BeginResize(label_w, -1.0f);
        {
            const UI_SETTINGS_ROW *const row = M_GetOptionByRow(s, row_idx);
            const char *const name =
                row != nullptr ? M_GetOptionTitle(row) : "";
            M_OptionLabel(row, name, true);
        }
        UI_EndResize();
        UI_Spacer(20.0f, 0.0f);

        UI_BeginResize(max_value_w, -1.0f);
        UI_BeginAnchor(1.0f, 0.5f);

        UI_BeginRowArrows(
            is_row_focused && M_CanChangeValue(s, row_idx, -1),
            is_row_focused && M_CanChangeValue(s, row_idx, +1),
            UI_ROW_ARROWS_MEDIUM);
        {
            const UI_SETTINGS_ROW *const row = M_GetOptionByRow(s, row_idx);
            if (M_IsBarColorEnum(row)) {
                UI_Bar((UI_BAR_SETTINGS) {
                    .w = M_BAR_WIDTH,
                    .h = M_BAR_HEIGHT,
                    .value = 100,
                    .max_value = 100,
                    .type = M_GetBarType(row),
                    .preview = true,
                });
            } else if (M_IsColorEditorOption(row)) {
                const char *const value = M_FormatRowValue(s, row_idx);
                const RGB_888 *const color = &row->option->value.as_rgb;
                UI_BeginStackEx((UI_STACK_SETTINGS) {
                    .orientation = UI_STACK_HORIZONTAL,
                    .align = { .v = UI_STACK_V_ALIGN_CENTER },
                    .spacing = { .h = 4.0f },
                });
                M_OptionLabel(row, value, false);
                UI_ColorSwatch((UI_COLOR_SWATCH_SETTINGS) {
                    .color = *color,
                    .w = UI_TEXT_HEIGHT - 2.0f,
                    .h = UI_TEXT_HEIGHT - 2.0f,
                });
                UI_EndStack();
            } else {
                const char *const value = M_FormatRowValue(s, row_idx);
                M_OptionLabel(row, value, false);
            }
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
    UI_EndStack();
}

void UI_SettingsEditor_DrawFooter(
    UI_SETTINGS_EDITOR_STATE *const s, const UI_SETTINGS_PHASE dialog_phase)
{
    const int32_t row_idx = UI_Scrollable_GetSelectedItem(&s->scroll);
    const UI_SETTINGS_ROW *const row = M_GetOptionByRow(s, row_idx);

    const bool can_edit_value = dialog_phase == UI_SETTINGS_PHASE_EDIT_SETTINGS
        && row_idx >= 0 && row != nullptr
        && row->option->value.type == TVT_RGB_888 && !M_IsOptionHeld(row);
    const bool can_examine = dialog_phase == UI_SETTINGS_PHASE_EDIT_SETTINGS
        && row_idx >= 0 && row != nullptr
        && M_GetOptionDescription(row) != nullptr
        && M_GetOptionTitle(row) != nullptr;
    const bool can_restore_default =
        dialog_phase == UI_SETTINGS_PHASE_EDIT_SETTINGS && row_idx >= 0
        && row != nullptr && !M_IsOptionHeld(row)
        && !Config_Option_IsAtDefault(row->option);

    UI_BeginStackEx((UI_STACK_SETTINGS) {
        .orientation = UI_STACK_HORIZONTAL,
        .align = { .h = UI_STACK_H_ALIGN_DISTRIBUTE },
        .spacing = { .h = 20 },
    });
    UI_BeginHide(!can_examine && !can_edit_value);
    if (can_edit_value) {
        UI_LabelFmt(
            "\\{input menu_confirm} %s",
            GS("general/settings/common/edit_value"));
    } else {
        UI_LabelFmt(
            "\\{input menu_show_info} %s",
            GS("general/settings/common/toggle_help"));
    }
    UI_EndHide();
    UI_BeginHide(!can_restore_default);
    UI_LabelFmt(
        "\\{input unbind_key} %s",
        GS("general/settings/common/restore_default"));
    UI_EndHide();
    UI_EndStack();
}
