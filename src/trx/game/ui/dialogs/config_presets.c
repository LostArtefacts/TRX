#include <trx/game/ui/dialogs/config_presets.h>

#include <trx/config.h>
#include <trx/config/common.h>
#include <trx/config/presets.h>
#include <trx/config/registry.h>
#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/core/utils.h>
#include <trx/game/game_strings/entries.h>
#include <trx/game/input.h>
#include <trx/game/ui.h>
#include <trx/game/ui/common.h>
#include <trx/game/ui/elements/anchor.h>
#include <trx/game/ui/elements/label.h>
#include <trx/game/ui/elements/modal.h>
#include <trx/game/ui/elements/pad.h>
#include <trx/game/ui/elements/requester.h>
#include <trx/game/ui/elements/resize.h>
#include <trx/game/ui/elements/scrollable_stack.h>
#include <trx/game/ui/elements/spacer.h>
#include <trx/game/ui/elements/stack.h>
#include <trx/game/ui/elements/window.h>
#include <trx/game/ui/scaler.h>
#include <trx/game/ui/scrollable.h>
#include <trx/game/ui/text.h>

#include <string.h>

#define M_CONFIRM_VISIBLE_ROWS 10
// A narrow list of changes should stay narrow; this is the floor, not the
// width.
#define M_CONFIRM_MIN_W 72.0f
#define M_CONFIRM_PAD 6.0f
#define M_LIST_ROW_SPACING 5.0f
// Measuring a text leaves off the spacing after its final glyph, which the
// wrapper still counts. Without this, a line given exactly its own measured
// width loses its last word to a second line.
#define M_MEASURE_SLACK 1.0f

typedef enum {
    M_PHASE_BROWSE,
    M_PHASE_CONFIRM,
    M_PHASE_NO_CHANGES,
    M_PHASE_APPLIED,
} M_PHASE;

struct UI_CONFIG_PRESETS_STATE {
    M_PHASE phase;
    UI_REQUESTER_STATE req;
    UI_SCROLLABLE confirm_scroll;
    int32_t selected_idx;
};

static const char *M_GetPresetKeyLabel(const char *const key)
{
    const CONFIG_OPTION *const opt = Config_FindOption(key);
    if (opt != nullptr) {
        const char *const label = Config_Option_GetTitle(opt);
        if (label != nullptr) {
            return label;
        }
    }
    return key;
}

static const char *M_GetPhaseMessage(const M_PHASE phase)
{
    switch (phase) {
    case M_PHASE_APPLIED:
        return GS("general/config_presets/applied");
    case M_PHASE_NO_CHANGES:
        return GS("general/config_presets/no_changes");
    default:
        return nullptr;
    }
}

// The result is the caller's to free.
static char *M_FormatTitle(UI_CONFIG_PRESETS_STATE *const s)
{
    const CONFIG_PRESET *const preset = Config_Presets_Get(s->selected_idx);
    const char *const preset_name =
        preset != nullptr ? GameString_Get(preset->name_gs) : "";
    return String_Format(GS("general/config_presets/title_fmt"), preset_name);
}

// Whether a preset's entry is one this game answers to. Every preset is
// offered whichever game is running, and a setting a game declares takes only
// the values that game offers, so what this one cannot take is passed over
// rather than shown as a change it would not make.
static bool M_IsSettingApplicable(
    const CONFIG_OPTION *const opt, const char *const value)
{
    return opt != nullptr && Config_Option_AcceptsString(opt, value)
        && strcmp(Config_Option_GetValueAsString(opt, false), value) != 0;
}

static int32_t M_GetChangedSettingCount(const int32_t preset_idx)
{
    const CONFIG_PRESET *const preset = Config_Presets_Get(preset_idx);
    if (preset == nullptr || preset->setting_count == 0) {
        return 0;
    }

    int32_t changed_count = 0;
    for (int32_t i = 0; i < preset->setting_count; i++) {
        const CONFIG_OPTION *const opt = Config_FindOption(preset->keys[i]);
        if (M_IsSettingApplicable(opt, preset->values[i])) {
            changed_count++;
        }
    }
    return changed_count;
}

// The value line of a row, reading "was \{button right} becomes". The result is
// the caller's to free: one row is measured for every changed setting, and the
// rotating format buffer is eight deep, so a preset that changes more than that
// would overwrite strings the caller still holds.
static char *M_FormatValueChange(
    const CONFIG_OPTION *const opt, const char *const target_raw)
{
    const char *const current_value = Config_Option_GetValueAsString(opt, true);
    char *const target_value =
        Config_Option_NormalizeValueString(opt, target_raw, true);
    char *const result =
        String_Format("  %s \\{button right} %s", current_value, target_value);
    Memory_Free(target_value);
    return result;
}

// Draw text wrapped to the given width, one label per line. The confirm list
// scrolls by line, so each line has to be a child of its own; a single label
// holding newlines would scroll as one item and outgrow the screen.
static void M_WrappedLines(const char *const text, const float max_width)
{
    char *const wrapped = UI_Text_WordWrap(text, 1.0f, max_width);
    if (wrapped == nullptr) {
        UI_Label(text);
        return;
    }
    // Each label copies the text it is given, so cutting the buffer up in place
    // is enough.
    char *line = wrapped;
    while (line != nullptr) {
        char *const end = strchr(line, '\n');
        if (end != nullptr) {
            *end = '\0';
        }
        UI_Label(line);
        line = end != nullptr ? end + 1 : nullptr;
    }
    Memory_Free(wrapped);
}

// Text wrapped to the given width as a single label. Labels render newlines, so
// anything that does not scroll can stay one node.
static void M_WrappedLabel(const char *const text, const float max_width)
{
    char *const wrapped = UI_Text_WordWrap(text, 1.0f, max_width);
    UI_Label(wrapped != nullptr ? wrapped : text);
    Memory_Free(wrapped);
}

// The width the dialog's text may occupy, in the units the layout sizes in. The
// modal's padding and the window's chrome come off what the screen allows, and
// what the phase shows sets the width below that, so a narrow list of changes
// keeps the dialog narrow rather than stretching it.
static float M_GetConfirmContentWidth(UI_CONFIG_PRESETS_STATE *const s)
{
    const float scale = UI_Scaler_GetTextScale();
    float budget = UI_GetSafeCanvasWidth()
        - (2.0f * M_CONFIRM_PAD + UI_Window_GetChromeWidth()) * scale;

    float natural = M_CONFIRM_MIN_W * scale;

    char *const title = M_FormatTitle(s);
    natural = MAX(natural, UI_Label_MeasureW(title));
    Memory_Free(title);

    const char *const message = M_GetPhaseMessage(s->phase);
    if (message != nullptr) {
        // The message carries a pad of its own inside the window.
        budget -= 2.0f * M_CONFIRM_PAD * scale;
        natural = MAX(natural, UI_Label_MeasureW(message));
    }

    const CONFIG_PRESET *const preset = Config_Presets_Get(s->selected_idx);
    if (s->phase == M_PHASE_CONFIRM && preset != nullptr) {
        for (int32_t i = 0; i < preset->setting_count; i++) {
            const CONFIG_OPTION *const opt = Config_FindOption(preset->keys[i]);
            if (!M_IsSettingApplicable(opt, preset->values[i])) {
                continue;
            }
            char *const value_change =
                M_FormatValueChange(opt, preset->values[i]);
            natural =
                MAX(natural,
                    UI_Label_MeasureW(M_GetPresetKeyLabel(preset->keys[i])));
            natural = MAX(natural, UI_Label_MeasureW(value_change));
            Memory_Free(value_change);
        }
    }

    natural += M_MEASURE_SLACK * scale;
    return MIN(natural, MAX(budget, M_CONFIRM_MIN_W * scale));
}

static void M_DrawConfirmRows(
    UI_CONFIG_PRESETS_STATE *const s, const float content_width)
{
    const CONFIG_PRESET *const preset = Config_Presets_Get(s->selected_idx);
    if (preset == nullptr) {
        return;
    }

    for (int32_t i = 0; i < preset->setting_count; i++) {
        const CONFIG_OPTION *const opt = Config_FindOption(preset->keys[i]);
        if (!M_IsSettingApplicable(opt, preset->values[i])) {
            continue;
        }
        char *const value_change = M_FormatValueChange(opt, preset->values[i]);
        M_WrappedLines(M_GetPresetKeyLabel(preset->keys[i]), content_width);
        M_WrappedLines(value_change, content_width);
        Memory_Free(value_change);
    }
}

static void M_Header(void *const user_data)
{
    UI_CONFIG_PRESETS_STATE *const s = user_data;
    if (s->phase == M_PHASE_CONFIRM) {
        M_WrappedLabel(
            GS("general/config_presets/confirm_description"),
            M_GetConfirmContentWidth(s));
        UI_Spacer(0.0f, UI_TEXT_HEIGHT);
    }
}

static void M_Footer(void *const user_data)
{
    UI_CONFIG_PRESETS_STATE *const s = user_data;
    if (s->phase == M_PHASE_CONFIRM) {
        UI_Spacer(0.0f, UI_TEXT_HEIGHT);
        M_WrappedLabel(
            GS("general/config_presets/confirm_restart_note"),
            M_GetConfirmContentWidth(s));
    }
}

UI_CONFIG_PRESETS_STATE *UI_ConfigPresets_Init(void)
{
    UI_CONFIG_PRESETS_STATE *const s =
        Memory_Alloc(sizeof(UI_CONFIG_PRESETS_STATE));
    s->phase = M_PHASE_BROWSE;
    s->selected_idx = -1;
    s->confirm_scroll = (UI_SCROLLABLE) {
        .first_item = 0,
        .sel_item = -1,
        .vis_items = M_CONFIRM_VISIBLE_ROWS,
        .max_items = 0,
    };

    const int32_t count = Config_Presets_GetCount();
    UI_Requester_Init(&s->req, count > 0 ? 1 : 0, count, true);
    UI_Requester_SelectRow(&s->req, -1);
    return s;
}

void UI_ConfigPresets_Free(UI_CONFIG_PRESETS_STATE *const s)
{
    UI_Requester_Free(&s->req);
    Memory_Free(s);
}

int32_t UI_ConfigPresets_GetItemCount(UI_CONFIG_PRESETS_STATE *const s)
{
    return Config_Presets_GetCount();
}

void UI_ConfigPresets_RecomputeSizes(
    UI_CONFIG_PRESETS_STATE *const s, const float max_content_height)
{
    int32_t clamped_rows = 0;
    const int32_t count = Config_Presets_GetCount();
    if (max_content_height > 0.0f) {
        clamped_rows = (max_content_height + M_LIST_ROW_SPACING)
            / (UI_TEXT_HEIGHT + M_LIST_ROW_SPACING);
    }
    if (clamped_rows > count) {
        clamped_rows = count;
    }
    UI_Requester_SetVisibleRows(&s->req, clamped_rows);
}

float UI_ConfigPresets_GetContentWidth(UI_CONFIG_PRESETS_STATE *const s)
{
    return -1.0f;
}

float UI_ConfigPresets_GetContentHeight(UI_CONFIG_PRESETS_STATE *const s)
{
    if (s == nullptr) {
        return -1.0f;
    }
    int32_t rows = s->req.scroll.vis_items;
    if (rows <= 0) {
        return -1.0f;
    }
    return rows * UI_TEXT_HEIGHT + (rows - 1) * M_LIST_ROW_SPACING;
}

UI_SCROLLABLE *UI_ConfigPresets_GetScrollable(UI_CONFIG_PRESETS_STATE *const s)
{
    return &s->req.scroll;
}

bool UI_ConfigPresets_Control(UI_CONFIG_PRESETS_STATE *const s)
{
    if (s->phase == M_PHASE_APPLIED || s->phase == M_PHASE_NO_CHANGES) {
        if (g_InputDB.menu_confirm || g_InputDB.menu_back) {
            s->phase = M_PHASE_BROWSE;
            g_Input = (INPUT_STATE) {};
            g_InputDB = (INPUT_STATE) {};
        }
        return false;
    }

    if (s->phase == M_PHASE_CONFIRM) {
        UI_ScrollableStack_Control(&s->confirm_scroll, UI_STACK_VERTICAL);
        if (g_InputDB.menu_confirm) {
            Config_Presets_Apply(s->selected_idx);
            s->phase = M_PHASE_APPLIED;
            g_Input = (INPUT_STATE) {};
            g_InputDB = (INPUT_STATE) {};
        } else if (g_InputDB.menu_back) {
            s->phase = M_PHASE_BROWSE;
            g_Input = (INPUT_STATE) {};
            g_InputDB = (INPUT_STATE) {};
        }
        return false;
    }

    const int32_t count = Config_Presets_GetCount();
    UI_Requester_SetMaxRows(&s->req, count);
    if (count <= 0) {
        return g_InputDB.menu_back || g_InputDB.menu_up
            || (g_InputDB.menu_down && g_Config.ui.enable_wraparound);
    }

    if (g_InputDB.menu_up && UI_Requester_GetCurrentRow(&s->req) <= 0) {
        return true;
    }
    if (g_InputDB.menu_down && g_Config.ui.enable_wraparound
        && UI_Requester_GetCurrentRow(&s->req) >= count - 1) {
        return true;
    }

    const int32_t choice = UI_Requester_Control(&s->req);
    if (choice == UI_REQUESTER_CANCEL || g_InputDB.menu_back) {
        return true;
    }
    if (choice >= 0 && choice < count) {
        s->selected_idx = choice;
        if (M_GetChangedSettingCount(choice) > 0) {
            s->confirm_scroll.first_item = 0;
            s->confirm_scroll.sel_item = -1;
            // The stack counts its own lines once it has them; wrapping decides
            // how many a row takes.
            s->confirm_scroll.max_items = 0;
            s->phase = M_PHASE_CONFIRM;
        } else {
            s->phase = M_PHASE_NO_CHANGES;
        }
        g_Input = (INPUT_STATE) {};
        g_InputDB = (INPUT_STATE) {};
    }
    return false;
}

void UI_ConfigPresets(UI_CONFIG_PRESETS_STATE *const s)
{
    const int32_t count = Config_Presets_GetCount();

    UI_BeginResize(-1.0f, -1.0f);
    UI_BeginStackEx((UI_STACK_SETTINGS) {
        .orientation = UI_STACK_VERTICAL,
        .align = { .h = UI_STACK_H_ALIGN_SPAN },
        .spacing = { .v = M_LIST_ROW_SPACING },
    });

    const int32_t first_row = UI_Requester_GetFirstRow(&s->req);
    for (int32_t i = 0; i < s->req.scroll.vis_items; i++) {
        const int32_t row = first_row + i;
        if (row < count) {
            const CONFIG_PRESET *const preset = Config_Presets_Get(row);
            UI_BeginRequesterRow(&s->req, row);
            UI_BeginAnchor(0.5f, 0.5f);
            UI_Label(preset != nullptr ? GameString_Get(preset->name_gs) : "");
            UI_EndAnchor();
            UI_EndRequesterRow(&s->req, row);
        } else {
            UI_Spacer(0.0f, UI_TEXT_HEIGHT);
        }
    }

    if (count <= 0) {
        UI_BeginAnchor(0.5f, 0.5f);
        UI_Label(GS("general/config_presets/empty"));
        UI_EndAnchor();
    }

    UI_EndStack();
    UI_EndResize();
}

void UI_ConfigPresetsApplyModal(UI_CONFIG_PRESETS_STATE *const s)
{
    if (s->phase == M_PHASE_BROWSE) {
        return;
    }

    const float content_width = M_GetConfirmContentWidth(s);

    char *const title = M_FormatTitle(s);
    char *const wrapped_title = UI_Text_WordWrap(title, 1.0f, content_width);

    UI_BeginModal(0.5f, 0.5f);
    UI_BeginPad(M_CONFIRM_PAD, M_CONFIRM_PAD);
    UI_BeginWindow((UI_WINDOW_SETTINGS) {
        .title = wrapped_title != nullptr ? wrapped_title : title,
        .scrollable =
            s->phase == M_PHASE_CONFIRM ? &s->confirm_scroll : nullptr,
        .title_spacing = -1.0f,
        .header_func = M_Header,
        .footer_func = M_Footer,
        .user_data = s,
        .heavy = true,
        .reserve_scroll_space = true,
    });

    const char *const message = M_GetPhaseMessage(s->phase);
    if (message != nullptr) {
        UI_BeginPad(M_CONFIRM_PAD, M_CONFIRM_PAD);
        M_WrappedLabel(message, content_width);
        UI_EndPad();
    } else if (s->phase == M_PHASE_CONFIRM) {
        UI_BeginScrollableStack(
            &s->confirm_scroll,
            (UI_SCROLLABLE_STACK_SETTINGS) {
                .orientation = UI_STACK_VERTICAL,
                .spacing = 2.0f,
            });
        M_DrawConfirmRows(s, content_width);
        UI_EndScrollableStack();
    }

    UI_EndWindow();
    UI_EndPad();
    UI_EndModal();
    Memory_Free(wrapped_title);
    Memory_Free(title);
}
