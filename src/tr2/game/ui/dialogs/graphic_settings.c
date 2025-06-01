#include "game/ui/dialogs/graphic_settings.h"

#include <libtrx/config.h>

static const UI_SETTINGS_ENUM_ENTRY m_CameraModeEnumEntries[] = {
    { CAMERA_MODE_TR1, GS_ID(GRAPHIC_SETTINGS_CAMERA_MODE_TR1) },
    { CAMERA_MODE_TR2, GS_ID(GRAPHIC_SETTINGS_CAMERA_MODE_TR2) },
    { -1, nullptr },
};

static const UI_SETTINGS_ENUM_ENTRY m_TextureFilterEnumEntries[] = {
    // clang-format off
    { GFX_TF_NN, GS_ID(MISC_OFF), },
    { GFX_TF_BILINEAR, GS_ID(GRAPHIC_SETTINGS_BILINEAR), },
    { -1, nullptr, },
    // clang-format on
};

static const UI_SETTINGS_ENUM_ENTRY m_LightingContrastEnumEntries[] = {
    // clang-format off
    { LIGHTING_CONTRAST_LOW, GS_ID(GRAPHIC_SETTINGS_LIGHTING_CONTRAST_LOW), },
    { LIGHTING_CONTRAST_MEDIUM, GS_ID(GRAPHIC_SETTINGS_LIGHTING_CONTRAST_MEDIUM), },
    { LIGHTING_CONTRAST_HIGH, GS_ID(GRAPHIC_SETTINGS_LIGHTING_CONTRAST_HIGH), },
    { -1, nullptr, },
    // clang-format on
};

static const UI_SETTINGS_ENUM_ENTRY m_RenderModeEnumEntries[] = {
    // clang-format off
    { RM_SOFTWARE, GS_ID(GRAPHIC_SETTINGS_RENDER_MODE_SOFTWARE), },
    { RM_HARDWARE, GS_ID(GRAPHIC_SETTINGS_RENDER_MODE_HARDWARE), },
    { -1, nullptr, },
    // clang-format on
};

static const UI_SETTINGS_ENUM_ENTRY m_AspectModeEnumEntries[] = {
    // clang-format off
    { AM_4_3, GS_ID(GRAPHIC_SETTINGS_ASPECT_MODE_4_3), },
    { AM_16_9, GS_ID(GRAPHIC_SETTINGS_ASPECT_MODE_16_9), },
    { AM_ANY, GS_ID(GRAPHIC_SETTINGS_ASPECT_MODE_ANY), },
    { -1, nullptr, },
    // clang-format on
};

static const UI_SETTINGS_OPTION m_VisualsOptions[] = {
    {
        .option_type = COT_INT32,
        .label_id = GS_ID(GRAPHIC_SETTINGS_FOG_START),
        .target = &g_Config.visuals.fog_start,
        .min_value = 1,
        .max_value = 100,
    },

    {
        .option_type = COT_INT32,
        .label_id = GS_ID(GRAPHIC_SETTINGS_FOG_END),
        .target = &g_Config.visuals.fog_end,
        .min_value = 1,
        .max_value = 100,
    },

    {
        .option_type = COT_RGB888,
        .label_id = GS_ID(GRAPHIC_SETTINGS_WATER_COLOR_R),
        .target = &g_Config.visuals.water_color,
        .min_value = 0,
        .max_value = 255,
        .delta_slow = 1,
        .delta_fast = 10,
        .misc = (void *)(intptr_t)0,
    },

    {
        .option_type = COT_RGB888,
        .label_id = GS_ID(GRAPHIC_SETTINGS_WATER_COLOR_G),
        .target = &g_Config.visuals.water_color,
        .min_value = 0,
        .max_value = 255,
        .delta_slow = 1,
        .delta_fast = 10,
        .misc = (void *)(intptr_t)1,
    },

    {
        .option_type = COT_RGB888,
        .label_id = GS_ID(GRAPHIC_SETTINGS_WATER_COLOR_B),
        .target = &g_Config.visuals.water_color,
        .min_value = 0,
        .max_value = 255,
        .delta_slow = 1,
        .delta_fast = 10,
        .misc = (void *)(intptr_t)2,
    },

    {
        .option_type = COT_ENUM,
        .label_id = GS_ID(GRAPHIC_SETTINGS_CAMERA_MODE),
        .target = &g_Config.visuals.camera_mode,
        .delta_slow = 1,
        .delta_fast = 1,
        .misc = m_CameraModeEnumEntries,
    },

    {
        .option_type = COT_INT32,
        .label_id = GS_ID(GRAPHIC_SETTINGS_FOV),
        .target = &g_Config.visuals.fov,
        .min_value = 30,
        .max_value = 150,
        .delta_slow = 1,
        .delta_fast = 10,
    },

    {
        .option_type = COT_BOOL,
        .label_id = GS_ID(GRAPHIC_SETTINGS_USE_PSX_FOV),
        .target = &g_Config.visuals.use_psx_fov,
    },

    {
        .target = nullptr,
    },
};

static const UI_SETTINGS_OPTION m_UIOptions[] = {
    {
        .option_type = COT_DOUBLE,
        .label_id = GS_ID(GRAPHIC_SETTINGS_UI_TEXT_SCALE),
        .target = &g_Config.ui.text_scale,
        .min_value = 50,
        .max_value = 200,
        .delta_slow = 10,
        .delta_fast = 10,
    },

    {
        .option_type = COT_DOUBLE,
        .label_id = GS_ID(GRAPHIC_SETTINGS_UI_BAR_SCALE),
        .target = &g_Config.ui.bar_scale,
        .min_value = 50,
        .max_value = 200,
        .delta_slow = 10,
        .delta_fast = 10,
    },

    {
        .option_type = COT_BOOL,
        .label_id = GS_ID(GRAPHIC_SETTINGS_UI_SCROLL_WRAPAROUND),
        .target = &g_Config.ui.enable_wraparound,
    },

    {
        .target = nullptr,
    },
};

static const UI_SETTINGS_OPTION m_RenderOptions[] = {
    {
        .option_type = COT_INT32,
        .label_id = GS_ID(GRAPHIC_SETTINGS_FPS),
        .target = &g_Config.rendering.fps,
        .min_value = 30,
        .max_value = 60,
        .delta_slow = 30,
        .delta_fast = 30,
    },

    {
        .option_type = COT_ENUM,
        .label_id = GS_ID(GRAPHIC_SETTINGS_TEXTURE_FILTER),
        .target = &g_Config.rendering.texture_filter,
        .delta_slow = 1,
        .delta_fast = 1,
        .misc = m_TextureFilterEnumEntries,
    },

    {
        .option_type = COT_BOOL,
        .label_id = GS_ID(GRAPHIC_SETTINGS_TRAPEZOID_FILTER),
        .target = &g_Config.rendering.enable_trapezoid_filter,
    },

    {
        .option_type = COT_BOOL,
        .label_id = GS_ID(GRAPHIC_SETTINGS_DEPTH_BUFFER),
        .target = &g_Config.rendering.enable_zbuffer,
    },

    {
        .option_type = COT_ENUM,
        .label_id = GS_ID(GRAPHIC_SETTINGS_LIGHTING_CONTRAST),
        .target = &g_Config.rendering.lighting_contrast,
        .delta_slow = 1,
        .delta_fast = 1,
        .misc = m_LightingContrastEnumEntries,
    },

    {
        .option_type = COT_ENUM,
        .label_id = GS_ID(GRAPHIC_SETTINGS_RENDER_MODE),
        .target = &g_Config.rendering.render_mode,
        .delta_slow = 1,
        .delta_fast = 1,
        .misc = m_RenderModeEnumEntries,
    },

    {
        .option_type = COT_ENUM,
        .label_id = GS_ID(GRAPHIC_SETTINGS_ASPECT_MODE),
        .target = &g_Config.rendering.aspect_mode,
        .delta_slow = 1,
        .delta_fast = 1,
        .misc = m_AspectModeEnumEntries,
    },

    {
        .option_type = COT_INT32,
        .label_id = GS_ID(GRAPHIC_SETTINGS_SCALER),
        .target = &g_Config.rendering.scaler,
        .min_value = 1,
        .max_value = 4,
        .delta_slow = 1,
        .delta_fast = 1,
    },

    {
        .option_type = COT_FLOAT,
        .label_id = GS_ID(GRAPHIC_SETTINGS_SIZER),
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

static const UI_SETTINGS_TAB m_Tabs[] = {
    { GS_ID(GRAPHIC_SETTINGS_VISUALS_TAB), m_VisualsOptions },
    { GS_ID(GRAPHIC_SETTINGS_UI_TAB), m_UIOptions },
    { GS_ID(GRAPHIC_SETTINGS_RENDERING_TAB), m_RenderOptions },
};

void UI_GraphicSettings_Init(UI_GRAPHIC_SETTINGS_STATE *const s)
{
    UI_Settings_InitWithTabs(
        s, GS_ID(GRAPHIC_SETTINGS_TITLE), sizeof(m_Tabs) / sizeof(m_Tabs[0]),
        m_Tabs);
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
