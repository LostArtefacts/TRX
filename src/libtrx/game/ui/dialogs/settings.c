#include "game/ui/dialogs/settings.h"

#include "config.h"
#include "debug.h"
#include "game/input.h"
#include "game/scaler.h"
#include "game/ui/elements/anchor.h"
#include "game/ui/elements/hide.h"
#include "game/ui/elements/label.h"
#include "game/ui/elements/modal.h"
#include "game/ui/elements/requester.h"
#include "game/ui/elements/resize.h"
#include "game/ui/elements/spacer.h"
#include "game/ui/elements/stack.h"
#include "game/viewport.h"
#include "memory.h"
#include "strings.h"
#include "utils.h"

#include <math.h>

#define M_ARROW_SPACING 2.0f

typedef struct {
    const UI_SETTINGS_ENUM_ENTRY *entry;
    int32_t position;
    int32_t count;
} M_ENUM_LOOKUP;

static int32_t M_GetVisibleRows(void);
static uint8_t *M_GetColorComponent(const UI_SETTINGS_OPTION *option);
static M_ENUM_LOOKUP M_GetEnumEntry(const UI_SETTINGS_OPTION *option);
static char *M_FormatRowValue(const UI_SETTINGS_STATE *s, int32_t row_idx);
static bool M_CanChangeValue(
    const UI_SETTINGS_STATE *s, int32_t row_idx, int32_t dir);
static bool M_RequestChangeValue(
    const UI_SETTINGS_STATE *s, int32_t row_idx, int32_t dir);
static float M_GetValueWidth(const UI_SETTINGS_STATE *s);

static int32_t M_GetVisibleRows(void)
{
    const int32_t res_h =
        Scaler_CalcInverse(Viewport_GetHeight(), SCALER_TARGET_TEXT);
    if (res_h <= 240) {
        return 5;
    } else if (res_h <= 384) {
        return 7;
    } else if (res_h < 480) {
        return 10;
    } else {
        return 12;
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

static char *M_FormatRowValue(
    const UI_SETTINGS_STATE *const s, const int32_t row_idx)
{
    const UI_SETTINGS_OPTION *const option = &s->options[row_idx];
    if (option->custom_handler.format_value != nullptr) {
        return option->custom_handler.format_value(option);
    }
    switch (option->option_type) {
    case COT_BOOL:
        return String_Format(
            "%s", *(bool *)option->target ? GS(MISC_ON) : GS(MISC_OFF));
    case COT_INT32:
        return String_Format(
            GS(DETAIL_INTEGER_FMT), *(int32_t *)option->target);
    case COT_DOUBLE:
        return String_Format(GS(DETAIL_FLOAT_FMT), *(double *)option->target);
    case COT_FLOAT:
        return String_Format(GS(DETAIL_FLOAT_FMT), *(float *)option->target);
    case COT_RGB888: {
        const uint8_t *const component = M_GetColorComponent(option);
        return String_Format("%d", *component);
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

static bool M_CanChangeValue(
    const UI_SETTINGS_STATE *const s, const int32_t row_idx, const int32_t dir)
{
    const UI_SETTINGS_OPTION *const option = &s->options[row_idx];
    if (option->custom_handler.can_change_value != nullptr) {
        return option->custom_handler.can_change_value(option, dir);
    }
    switch (option->option_type) {
    case COT_BOOL:
        return true;
    case COT_INT32:
        if (dir < 0) {
            return *(int32_t *)option->target > option->min_value;
        } else if (dir > 0) {
            return *(int32_t *)option->target < option->max_value;
        }
        break;
    case COT_DOUBLE:
        if (dir < 0) {
            return *(double *)option->target > (double)option->min_value / 100;
        } else if (dir > 0) {
            return *(double *)option->target < (double)option->max_value / 100;
        }
        break;
    case COT_FLOAT:
        if (dir < 0) {
            return *(float *)option->target > (float)option->min_value / 100;
        } else if (dir > 0) {
            return *(float *)option->target < (float)option->max_value / 100;
        }
        break;
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
    int32_t delta = g_Input.slow ? option->delta_slow : option->delta_fast;
    if (delta == 0) {
        delta = 1;
    }
    delta *= dir;

    if (option->custom_handler.request_change_value != nullptr) {
        if (option->custom_handler.request_change_value(option, dir)) {
            goto changed;
        }
        return false;
    }

    switch (option->option_type) {
    case COT_BOOL:
        *(bool *)option->target = !*(bool *)option->target;
        break;
    case COT_INT32:
        *(int32_t *)option->target += delta;
        break;
    case COT_DOUBLE:
        *(double *)option->target =
            (round(*(double *)option->target * 10) + (delta / 10.0f)) / 10.0f;
        break;
    case COT_FLOAT:
        *(float *)option->target =
            (round(*(float *)option->target * 10) + (delta / 10.0f)) / 10.0f;
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
    default:
        return false;
    }

changed:
    Config_Write();
    return true;
}

static float M_GetValueWidth(const UI_SETTINGS_STATE *const s)
{
    // Measure the maximum width of the value label to prevent the entire
    // dialog from changing its size as the player changes the levels.
    float result = -1.0f;
    for (int32_t i = 0; i < s->req.scroll.max_items; i++) {
        const UI_SETTINGS_OPTION *const option = &s->options[i];
        if (option->option_type == COT_ENUM) {
            const UI_SETTINGS_ENUM_ENTRY *entry =
                &((UI_SETTINGS_ENUM_ENTRY *)option->misc)[0];
            while (entry->value != -1) {
                const char *const value = GameString_Get(entry->name);
                float value_w;
                UI_Label_Measure(value, &value_w, nullptr);
                result = MAX(result, value_w);
                entry++;
            }
        } else {
            const char *const value = M_FormatRowValue(s, i);
            float value_w;
            UI_Label_Measure(value, &value_w, nullptr);
            result = MAX(result, value_w);
        }
    }
    float arrow_w;
    UI_Label_Measure("\\{button left}", &arrow_w, nullptr);
    result += arrow_w;
    UI_Label_Measure("\\{button right}", &arrow_w, nullptr);
    result += arrow_w;
    result += M_ARROW_SPACING * 2;
    return result;
}

void UI_Settings_Init(
    UI_SETTINGS_STATE *const s, const UI_SETTINGS_OPTION *const options)
{
    s->options = options;
    int32_t row_count = 0;
    for (int32_t i = 0; s->options[i].label_id != nullptr; i++) {
        row_count++;
    }
    UI_Requester_Init(&s->req, row_count, row_count, true);
    s->req.row_pad = 2.0f;
    s->req.row_spacing = 0.0f;
    s->req.show_arrows = true;
}

void UI_Settings_Free(UI_SETTINGS_STATE *const s)
{
    UI_Requester_Free(&s->req);
}

bool UI_Settings_Control(UI_SETTINGS_STATE *const s)
{
    UI_Requester_SetVisibleRows(&s->req, M_GetVisibleRows());
    const int32_t choice = UI_Requester_Control(&s->req);
    if (choice == UI_REQUESTER_CANCEL) {
        return true;
    }
    const int32_t sel_row = UI_Requester_GetCurrentRow(&s->req);
    if (g_InputDB.menu_left && sel_row >= 0) {
        M_RequestChangeValue(s, sel_row, -1);
    } else if (g_InputDB.menu_right && sel_row >= 0) {
        M_RequestChangeValue(s, sel_row, +1);
    }
    return false;
}

void UI_Settings(UI_SETTINGS_STATE *const s)
{
    const int32_t sel_row = UI_Requester_GetCurrentRow(&s->req);
    UI_BeginModal(0.5f, 0.6f);
    UI_BeginRequester(&s->req, GS(DETAIL_TITLE));

    const float max_value_w = M_GetValueWidth(s) / g_Config.ui.text_scale;

    for (int32_t i = 0; i < s->req.scroll.max_items; i++) {
        if (!UI_Requester_IsRowVisible(&s->req, i)) {
            UI_BeginResize(-1.0f, 0.0f);
        } else {
            UI_BeginResize(-1.0f, -1.0f);
        }

        UI_BeginRequesterRow(&s->req, i);
        UI_BeginStackEx((UI_STACK_SETTINGS) {
            .orientation = UI_STACK_HORIZONTAL,
            .align = { .h = UI_STACK_H_ALIGN_DISTRIBUTE },
        });
        UI_Label(GameString_Get(s->options[i].label_id));
        UI_Spacer(20.0f, 0.0f);

        UI_BeginResize(max_value_w, -1.0f);
        UI_BeginAnchor(1.0f, 0.5f);
        UI_BeginStackEx((UI_STACK_SETTINGS) {
            .orientation = UI_STACK_HORIZONTAL,
            .align = { .h = UI_STACK_H_ALIGN_DISTRIBUTE },
            .spacing = { .h = M_ARROW_SPACING },
        });
        UI_BeginHide(i != sel_row || !M_CanChangeValue(s, i, -1));
        UI_Label("\\{button left}");
        UI_EndHide();

        UI_Label(M_FormatRowValue(s, i));

        UI_BeginHide(i != sel_row || !M_CanChangeValue(s, i, +1));
        UI_Label("\\{button right}");
        UI_EndHide();
        UI_EndStack();
        UI_EndAnchor();
        UI_EndResize();

        UI_EndStack();
        UI_EndRequesterRow(&s->req, i);
        UI_EndResize();
    }
    UI_EndRequester(&s->req);
    UI_EndModal();
}
