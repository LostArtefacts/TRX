#include "game/ui/dialogs/graphic_settings.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/game/game_string.h>
#include <libtrx/game/input.h>
#include <libtrx/game/scaler.h>
#include <libtrx/game/ui/elements/anchor.h>
#include <libtrx/game/ui/elements/hide.h>
#include <libtrx/game/ui/elements/label.h>
#include <libtrx/game/ui/elements/modal.h>
#include <libtrx/game/ui/elements/requester.h>
#include <libtrx/game/ui/elements/resize.h>
#include <libtrx/game/ui/elements/spacer.h>
#include <libtrx/game/ui/elements/stack.h>
#include <libtrx/game/viewport.h>
#include <libtrx/memory.h>
#include <libtrx/strings.h>
#include <libtrx/utils.h>

#include <math.h>

#define M_ARROW_SPACING 2.0f

typedef struct {
    CONFIG_OPTION_TYPE option_type;
    GAME_STRING_ID label_id;
    void *target;
    int32_t min_value;
    int32_t max_value;
    int32_t delta_slow;
    int32_t delta_fast;
    const void *misc;
} M_OPTION;

typedef struct {
    int32_t value;
    GAME_STRING_ID name;
} M_ENUM_ENTRY;

typedef struct {
    const M_ENUM_ENTRY *entry;
    int32_t position;
    int32_t count;
} M_ENUM_LOOKUP;

static const M_ENUM_ENTRY m_TextureFilterOptions[] = {
    // clang-format off
    { GFX_TF_NN, GS_ID(MISC_OFF), },
    { GFX_TF_BILINEAR, GS_ID(DETAIL_BILINEAR), },
    { -1, nullptr, },
    // clang-format on
};

static const M_ENUM_ENTRY m_LightingContrastOptions[] = {
    // clang-format off
    { LIGHTING_CONTRAST_LOW, GS_ID(DETAIL_LIGHTING_CONTRAST_LOW), },
    { LIGHTING_CONTRAST_MEDIUM, GS_ID(DETAIL_LIGHTING_CONTRAST_MEDIUM), },
    { LIGHTING_CONTRAST_HIGH, GS_ID(DETAIL_LIGHTING_CONTRAST_HIGH), },
    { -1, nullptr, },
    // clang-format on
};

static const M_ENUM_ENTRY m_RenderModeOptions[] = {
    // clang-format off
    { RM_SOFTWARE, GS_ID(DETAIL_RENDER_MODE_SOFTWARE), },
    { RM_HARDWARE, GS_ID(DETAIL_RENDER_MODE_HARDWARE), },
    { -1, nullptr, },
    // clang-format on
};

static const M_ENUM_ENTRY m_AspectModeOptions[] = {
    // clang-format off
    { AM_4_3, GS_ID(DETAIL_ASPECT_MODE_4_3), },
    { AM_16_9, GS_ID(DETAIL_ASPECT_MODE_16_9), },
    { AM_ANY, GS_ID(DETAIL_ASPECT_MODE_ANY), },
    { -1, nullptr, },
    // clang-format on
};

static const M_OPTION m_Options[] = {
    {
        .option_type = COT_INT32,
        .label_id = GS_ID(DETAIL_FPS),
        .target = &g_Config.rendering.fps,
        .min_value = 30,
        .max_value = 60,
        .delta_slow = 30,
        .delta_fast = 30,
    },

    {
        .option_type = COT_INT32,
        .label_id = GS_ID(DETAIL_FOG_START),
        .target = &g_Config.visuals.fog_start,
        .min_value = 1,
        .max_value = 100,
    },

    {
        .option_type = COT_INT32,
        .label_id = GS_ID(DETAIL_FOG_END),
        .target = &g_Config.visuals.fog_end,
        .min_value = 1,
        .max_value = 100,
    },

    {
        .option_type = COT_RGB888,
        .label_id = GS_ID(DETAIL_WATER_COLOR_R),
        .target = &g_Config.visuals.water_color,
        .min_value = 0,
        .max_value = 255,
        .delta_slow = 1,
        .delta_fast = 10,
        .misc = (void *)(intptr_t)0,
    },

    {
        .option_type = COT_RGB888,
        .label_id = GS_ID(DETAIL_WATER_COLOR_G),
        .target = &g_Config.visuals.water_color,
        .min_value = 0,
        .max_value = 255,
        .delta_slow = 1,
        .delta_fast = 10,
        .misc = (void *)(intptr_t)1,
    },

    {
        .option_type = COT_RGB888,
        .label_id = GS_ID(DETAIL_WATER_COLOR_B),
        .target = &g_Config.visuals.water_color,
        .min_value = 0,
        .max_value = 255,
        .delta_slow = 1,
        .delta_fast = 10,
        .misc = (void *)(intptr_t)2,
    },

    {
        .option_type = COT_INT32,
        .label_id = GS_ID(DETAIL_FOV),
        .target = &g_Config.visuals.fov,
        .min_value = 30,
        .max_value = 150,
        .delta_slow = 1,
        .delta_fast = 10,
    },

    {
        .option_type = COT_BOOL,
        .label_id = GS_ID(DETAIL_USE_PSX_FOV),
        .target = &g_Config.visuals.use_psx_fov,
    },

    {
        .option_type = COT_ENUM,
        .label_id = GS_ID(DETAIL_TEXTURE_FILTER),
        .target = &g_Config.rendering.texture_filter,
        .delta_slow = 1,
        .delta_fast = 1,
        .misc = m_TextureFilterOptions,
    },

    {
        .option_type = COT_BOOL,
        .label_id = GS_ID(DETAIL_TRAPEZOID_FILTER),
        .target = &g_Config.rendering.enable_trapezoid_filter,
    },

    {
        .option_type = COT_BOOL,
        .label_id = GS_ID(DETAIL_DEPTH_BUFFER),
        .target = &g_Config.rendering.enable_zbuffer,
    },

    {
        .option_type = COT_ENUM,
        .label_id = GS_ID(DETAIL_LIGHTING_CONTRAST),
        .target = &g_Config.rendering.lighting_contrast,
        .delta_slow = 1,
        .delta_fast = 1,
        .misc = m_LightingContrastOptions,
    },

    {
        .option_type = COT_ENUM,
        .label_id = GS_ID(DETAIL_RENDER_MODE),
        .target = &g_Config.rendering.render_mode,
        .delta_slow = 1,
        .delta_fast = 1,
        .misc = m_RenderModeOptions,
    },

    {
        .option_type = COT_ENUM,
        .label_id = GS_ID(DETAIL_ASPECT_MODE),
        .target = &g_Config.rendering.aspect_mode,
        .delta_slow = 1,
        .delta_fast = 1,
        .misc = m_AspectModeOptions,
    },

    {
        .option_type = COT_DOUBLE,
        .label_id = GS_ID(DETAIL_UI_TEXT_SCALE),
        .target = &g_Config.ui.text_scale,
        .min_value = 50,
        .max_value = 200,
        .delta_slow = 10,
        .delta_fast = 10,
    },

    {
        .option_type = COT_DOUBLE,
        .label_id = GS_ID(DETAIL_UI_BAR_SCALE),
        .target = &g_Config.ui.bar_scale,
        .min_value = 50,
        .max_value = 200,
        .delta_slow = 10,
        .delta_fast = 10,
    },

    {
        .option_type = COT_BOOL,
        .label_id = GS_ID(DETAIL_UI_SCROLL_WRAPAROUND),
        .target = &g_Config.ui.enable_wraparound,
    },

    {
        .option_type = COT_INT32,
        .label_id = GS_ID(DETAIL_SCALER),
        .target = &g_Config.rendering.scaler,
        .min_value = 1,
        .max_value = 4,
        .delta_slow = 1,
        .delta_fast = 1,
    },

    {
        .option_type = COT_FLOAT,
        .label_id = GS_ID(DETAIL_SIZER),
        .target = &g_Config.rendering.sizer,
        .min_value = 40,
        .max_value = 200,
        .delta_slow = 10,
        .delta_fast = 10,
    },

    {
        .target = nullptr,
    },
};

static int32_t M_GetVisibleRows(void);
static uint8_t *M_GetColorComponent(const M_OPTION *option);
static M_ENUM_LOOKUP M_GetEnumEntry(const M_OPTION *option);
static char *M_FormatRowValue(int32_t row_idx);
static bool M_CanChangeValue(int32_t row_idx, int32_t dir);
static bool M_RequestChangeValue(int32_t row_idx, int32_t dir);
static float M_GetValueWidth(const UI_GRAPHIC_SETTINGS_STATE *s);

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

static uint8_t *M_GetColorComponent(const M_OPTION *const option)
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

static M_ENUM_LOOKUP M_GetEnumEntry(const M_OPTION *const option)
{
    M_ENUM_LOOKUP result = {
        .entry = nullptr,
        .position = -1,
        .count = 0,
    };
    int32_t current_pos = 0;
    const M_ENUM_ENTRY *entry = &((M_ENUM_ENTRY *)option->misc)[0];
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

static char *M_FormatRowValue(const int32_t row_idx)
{
    const M_OPTION *const option = &m_Options[row_idx];
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

static bool M_CanChangeValue(const int32_t row_idx, const int32_t dir)
{
    const M_OPTION *const option = &m_Options[row_idx];
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

static bool M_RequestChangeValue(const int32_t row_idx, const int32_t dir)
{
    if (!M_CanChangeValue(row_idx, dir)) {
        return false;
    }

    const M_OPTION *const option = &m_Options[row_idx];
    int32_t delta = g_Input.slow ? option->delta_slow : option->delta_fast;
    if (delta == 0) {
        delta = 1;
    }
    delta *= dir;

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
        const M_ENUM_ENTRY *const next_entry =
            &((M_ENUM_ENTRY *)option->misc)[enum_lookup.position + delta];
        *(int32_t *)option->target = next_entry->value;
        break;
    }
    default:
        return false;
    }
    Config_Write();
    return true;
}

static float M_GetValueWidth(const UI_GRAPHIC_SETTINGS_STATE *const s)
{
    // Measure the maximum width of the value label to prevent the entire
    // dialog from changing its size as the player changes the sound levels.
    float result = -1.0f;
    for (int32_t i = 0; i < s->req.max_rows; i++) {
        const char *const value = M_FormatRowValue(i);
        float value_w;
        UI_Label_Measure(value, &value_w, nullptr);
        result = MAX(result, value_w);
    }
    float arrow_w;
    UI_Label_Measure("\\{button left}", &arrow_w, nullptr);
    result += arrow_w;
    UI_Label_Measure("\\{button right}", &arrow_w, nullptr);
    result += arrow_w;
    result += M_ARROW_SPACING * 2;
    return result;
}

void UI_GraphicSettings_Init(UI_GRAPHIC_SETTINGS_STATE *const s)
{
    int32_t row_count = 0;
    for (int32_t i = 0; m_Options[i].target != nullptr; i++) {
        row_count++;
    }
    UI_Requester_Init(&s->req, row_count, row_count, true);
    s->req.row_pad = 2.0f;
    s->req.row_spacing = 0.0f;
    s->req.show_arrows = true;
}

void UI_GraphicSettings_Free(UI_GRAPHIC_SETTINGS_STATE *const s)
{
    UI_Requester_Free(&s->req);
}

bool UI_GraphicSettings_Control(UI_GRAPHIC_SETTINGS_STATE *const s)
{
    UI_Requester_SetVisibleRows(&s->req, M_GetVisibleRows());
    const int32_t choice = UI_Requester_Control(&s->req);
    if (choice == UI_REQUESTER_CANCEL) {
        return true;
    }
    const int32_t sel_row = UI_Requester_GetCurrentRow(&s->req);
    if (g_InputDB.menu_left && sel_row >= 0) {
        M_RequestChangeValue(sel_row, -1);
    } else if (g_InputDB.menu_right && sel_row >= 0) {
        M_RequestChangeValue(sel_row, +1);
    }
    return false;
}

void UI_GraphicSettings(UI_GRAPHIC_SETTINGS_STATE *const s)
{
    const int32_t sel_row = UI_Requester_GetCurrentRow(&s->req);
    UI_BeginModal(0.5f, 0.6f);
    UI_BeginRequester(&s->req, GS(DETAIL_TITLE));

    const float max_value_w = M_GetValueWidth(s) / g_Config.ui.text_scale;

    for (int32_t i = 0; i < s->req.max_rows; i++) {
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
        UI_Label(GameString_Get(m_Options[i].label_id));
        UI_Spacer(20.0f, 0.0f);

        UI_BeginResize(max_value_w, -1.0f);
        UI_BeginAnchor(1.0f, 0.5f);
        UI_BeginStackEx((UI_STACK_SETTINGS) {
            .orientation = UI_STACK_HORIZONTAL,
            .align = { .h = UI_STACK_H_ALIGN_DISTRIBUTE },
            .spacing = { .h = M_ARROW_SPACING },
        });
        UI_BeginHide(i != sel_row || !M_CanChangeValue(i, -1));
        UI_Label("\\{button left}");
        UI_EndHide();

        UI_Label(M_FormatRowValue(i));

        UI_BeginHide(i != sel_row || !M_CanChangeValue(i, +1));
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
