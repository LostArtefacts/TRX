#include "game/ui/dialogs/graphic_settings.h"

#include <libtrx/config.h>

static const UI_SETTINGS_ENUM_ENTRY m_TextureFilterOptions[] = {
    // clang-format off
    { GFX_TF_NN, GS_ID(MISC_OFF), },
    { GFX_TF_BILINEAR, GS_ID(DETAIL_BILINEAR), },
    { -1, nullptr, },
    // clang-format on
};

static const UI_SETTINGS_ENUM_ENTRY m_LightingContrastOptions[] = {
    // clang-format off
    { LIGHTING_CONTRAST_LOW, GS_ID(DETAIL_LIGHTING_CONTRAST_LOW), },
    { LIGHTING_CONTRAST_MEDIUM, GS_ID(DETAIL_LIGHTING_CONTRAST_MEDIUM), },
    { LIGHTING_CONTRAST_HIGH, GS_ID(DETAIL_LIGHTING_CONTRAST_HIGH), },
    { -1, nullptr, },
    // clang-format on
};

static const UI_SETTINGS_ENUM_ENTRY m_RenderModeOptions[] = {
    // clang-format off
    { RM_SOFTWARE, GS_ID(DETAIL_RENDER_MODE_SOFTWARE), },
    { RM_HARDWARE, GS_ID(DETAIL_RENDER_MODE_HARDWARE), },
    { -1, nullptr, },
    // clang-format on
};

static const UI_SETTINGS_ENUM_ENTRY m_AspectModeOptions[] = {
    // clang-format off
    { AM_4_3, GS_ID(DETAIL_ASPECT_MODE_4_3), },
    { AM_16_9, GS_ID(DETAIL_ASPECT_MODE_16_9), },
    { AM_ANY, GS_ID(DETAIL_ASPECT_MODE_ANY), },
    { -1, nullptr, },
    // clang-format on
};

static const UI_SETTINGS_OPTION m_Options[] = {
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

void UI_GraphicSettings_Init(UI_GRAPHIC_SETTINGS_STATE *const s)
{
    UI_Settings_Init(s, m_Options);
}

void UI_GraphicSettings_Free(UI_GRAPHIC_SETTINGS_STATE *const s)
{
    UI_Settings_Free(s);
}

bool UI_GraphicSettings_Control(UI_GRAPHIC_SETTINGS_STATE *const s)
{
    return UI_Settings_Control(s);
}

void UI_GraphicSettings(UI_GRAPHIC_SETTINGS_STATE *const s)
{
    UI_Settings(s);
}
