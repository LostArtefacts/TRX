#include "game/ui/dialogs/graphic_settings.h"

#include "game/screen.h"

#include <libtrx/config.h>
#include <libtrx/strings.h>

static const UI_SETTINGS_ENUM_ENTRY m_TextureFilterOptions[] = {
    { GFX_TF_NN, GS_ID(MISC_OFF) },
    { GFX_TF_BILINEAR, GS_ID(DETAIL_BILINEAR) },
    { -1, nullptr },
};

static const UI_SETTINGS_ENUM_ENTRY m_RenderModeOptions[] = {
    { GFX_RM_LEGACY, GS_ID(DETAIL_RENDER_MODE_LEGACY) },
    { GFX_RM_FRAMEBUFFER, GS_ID(DETAIL_RENDER_MODE_FBO) },
    { -1, nullptr },
};

static char *M_ScreenResolution_FormatValue(const UI_SETTINGS_OPTION *option);
static bool M_ScreenResolution_CanChangeValue(
    const UI_SETTINGS_OPTION *option, int32_t dir);
static bool M_ScreenResolution_RequestChangeValue(
    const UI_SETTINGS_OPTION *option, int32_t dir);

static char *M_ScreenResolution_FormatValue(
    const UI_SETTINGS_OPTION *const option)
{
    return String_Format(
        GS(DETAIL_RESOLUTION_FMT), Screen_GetResWidth(), Screen_GetResHeight());
}

static bool M_ScreenResolution_CanChangeValue(
    const UI_SETTINGS_OPTION *const option, const int32_t dir)
{
    return dir < 0 ? Screen_CanSetPrevRes() : Screen_CanSetNextRes();
}

static bool M_ScreenResolution_RequestChangeValue(
    const UI_SETTINGS_OPTION *const option, const int32_t dir)
{
    if (dir < 0) {
        Screen_SetPrevRes();
    } else {
        Screen_SetNextRes();
    }
    return true;
}

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
        .target = &g_Config.visuals.fov_value,
        .min_value = 30,
        .max_value = 150,
        .delta_slow = 1,
        .delta_fast = 10,
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
        .option_type = COT_ENUM,
        .label_id = GS_ID(DETAIL_FBO_FILTER),
        .target = &g_Config.rendering.fbo_filter,
        .delta_slow = 1,
        .delta_fast = 1,
        .misc = m_TextureFilterOptions,
    },

    {
        .label_id = GS_ID(DETAIL_RESOLUTION),
        .custom_handler = {
            .format_value = M_ScreenResolution_FormatValue,
            .can_change_value = M_ScreenResolution_CanChangeValue,
            .request_change_value = M_ScreenResolution_RequestChangeValue,
        },
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
        .option_type = COT_BOOL,
        .label_id = GS_ID(DETAIL_TRAPEZOID_FILTER),
        .target = &g_Config.rendering.enable_trapezoid_filter,
    },

    {
        .option_type = COT_BOOL,
        .label_id = GS_ID(DETAIL_REFLECTIONS),
        .target = &g_Config.visuals.enable_reflections,
    },

    {
        .option_type = COT_BOOL,
        .label_id = GS_ID(DETAIL_VSYNC),
        .target = &g_Config.rendering.enable_vsync,
    },

    {
        .option_type = COT_FLOAT,
        .label_id = GS_ID(DETAIL_BRIGHTNESS),
        .target = &g_Config.visuals.brightness,
        .min_value = CONFIG_MIN_BRIGHTNESS * 100,
        .max_value = CONFIG_MAX_BRIGHTNESS * 100,
        .delta_slow = 10,
        .delta_fast = 10,
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
