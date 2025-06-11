#include "game/ui/dialogs/graphic_settings.h"

#include "config.h"
#include "game/game_string_manager.h"
#include "memory.h"
#include "strings.h"

#include <stdlib.h>

static const UI_SETTINGS_ENUM_ENTRY m_HealthBarShowModeEnumEntries[] = {
    { BSM_DEFAULT, GS_ID(GRAPHIC_SETTINGS_BAR_MODE_DEFAULT) },
    { BSM_PS1, GS_ID(GRAPHIC_SETTINGS_BAR_MODE_PS1) },
    { BSM_FLASHING_OR_DEFAULT,
      GS_ID(GRAPHIC_SETTINGS_BAR_MODE_FLASHING_OR_DEFAULT) },
    { BSM_FLASHING_ONLY, GS_ID(GRAPHIC_SETTINGS_BAR_MODE_FLASHING_ONLY) },
    { BSM_ALWAYS, GS_ID(GRAPHIC_SETTINGS_BAR_MODE_ALWAYS) },
    { BSM_NEVER, GS_ID(GRAPHIC_SETTINGS_BAR_MODE_NEVER) },
    { -1, nullptr },
};

static const UI_SETTINGS_ENUM_ENTRY m_AirBarShowModeEnumEntries[] = {
    { BSM_DEFAULT, GS_ID(GRAPHIC_SETTINGS_BAR_MODE_DEFAULT) },
    { BSM_PS1, GS_ID(GRAPHIC_SETTINGS_BAR_MODE_PS1) },
    { BSM_FLASHING_ONLY, GS_ID(GRAPHIC_SETTINGS_BAR_MODE_FLASHING_ONLY) },
    { BSM_NEVER, GS_ID(GRAPHIC_SETTINGS_BAR_MODE_NEVER) },
    { -1, nullptr },
};

static const UI_SETTINGS_ENUM_ENTRY m_EnemyHealthBarShowModeEnumEntries[] = {
    { BSM_ALWAYS, GS_ID(GRAPHIC_SETTINGS_BAR_MODE_ALWAYS) },
    { BSM_NEVER, GS_ID(GRAPHIC_SETTINGS_BAR_MODE_NEVER) },
    { BSM_BOSS_ONLY, GS_ID(GRAPHIC_SETTINGS_BAR_MODE_BOSS_ONLY) },
    { -1, nullptr },
};

static const UI_SETTINGS_ENUM_ENTRY m_BarLocationEnumEntries[] = {
    { BL_TOP_LEFT, GS_ID(GRAPHIC_SETTINGS_BAR_LOCATION_TOP_LEFT) },
    { BL_TOP_CENTER, GS_ID(GRAPHIC_SETTINGS_BAR_LOCATION_TOP_CENTER) },
    { BL_TOP_RIGHT, GS_ID(GRAPHIC_SETTINGS_BAR_LOCATION_TOP_RIGHT) },
    { BL_BOTTOM_LEFT, GS_ID(GRAPHIC_SETTINGS_BAR_LOCATION_BOTTOM_LEFT) },
    { BL_BOTTOM_CENTER, GS_ID(GRAPHIC_SETTINGS_BAR_LOCATION_BOTTOM_CENTER) },
    { BL_BOTTOM_RIGHT, GS_ID(GRAPHIC_SETTINGS_BAR_LOCATION_BOTTOM_RIGHT) },
    { -1, nullptr },
};

static const UI_SETTINGS_ENUM_ENTRY m_BarColorEnumEntries[] = {
    {
        BC_GOLD,
        GS_ID(GRAPHIC_SETTINGS_BAR_COLOR_GOLD),
    },
    {
        BC_BLUE,
        GS_ID(GRAPHIC_SETTINGS_BAR_COLOR_BLUE),
    },
    {
        BC_GREY,
        GS_ID(GRAPHIC_SETTINGS_BAR_COLOR_GREY),
    },
    {
        BC_RED,
        GS_ID(GRAPHIC_SETTINGS_BAR_COLOR_RED),
    },
    {
        BC_SILVER,
        GS_ID(GRAPHIC_SETTINGS_BAR_COLOR_SILVER),
    },
    {
        BC_GREEN,
        GS_ID(GRAPHIC_SETTINGS_BAR_COLOR_GREEN),
    },
    {
        BC_GOLD2,
        GS_ID(GRAPHIC_SETTINGS_BAR_COLOR_GOLD2),
    },
    {
        BC_BLUE2,
        GS_ID(GRAPHIC_SETTINGS_BAR_COLOR_BLUE2),
    },
    { BC_PINK, GS_ID(GRAPHIC_SETTINGS_BAR_COLOR_PINK) },
    { BC_PURPLE, GS_ID(GRAPHIC_SETTINGS_BAR_COLOR_PURPLE) },
    { -1, nullptr },
};

static const UI_SETTINGS_ENUM_ENTRY m_CameraModeEnumEntries[] = {
    { CAMERA_MODE_TR1, GS_ID(GRAPHIC_SETTINGS_CAMERA_MODE_TR1) },
    { CAMERA_MODE_TR2, GS_ID(GRAPHIC_SETTINGS_CAMERA_MODE_TR2) },
    { -1, nullptr },
};

static const UI_SETTINGS_ENUM_ENTRY m_TextureFilterEnumEntries[] = {
    { GFX_TF_NN, GS_ID(MISC_OFF) },
    { GFX_TF_BILINEAR, GS_ID(GRAPHIC_SETTINGS_BILINEAR) },
    { -1, nullptr },
};

#if TR_VERSION == 1

static const UI_SETTINGS_ENUM_ENTRY m_RenderModeEnumEntries[] = {
    { GFX_RM_LEGACY, GS_ID(GRAPHIC_SETTINGS_RENDER_MODE_LEGACY) },
    { GFX_RM_FRAMEBUFFER, GS_ID(GRAPHIC_SETTINGS_RENDER_MODE_FBO) },
    { -1, nullptr },
};

static const UI_SETTINGS_ENUM_ENTRY m_MenuStyleEnumEntries[] = {
    { UI_STYLE_PS1, GS_ID(GRAPHIC_SETTINGS_UI_STYLE_PS1) },
    { UI_STYLE_PC, GS_ID(GRAPHIC_SETTINGS_UI_STYLE_PC) },
    { -1, nullptr },
};

#elif TR_VERSION == 2

static const UI_SETTINGS_ENUM_ENTRY m_LightingContrastEnumEntries[] = {
    { LIGHTING_CONTRAST_LOW, GS_ID(GRAPHIC_SETTINGS_LIGHTING_CONTRAST_LOW) },
    { LIGHTING_CONTRAST_MEDIUM,
      GS_ID(GRAPHIC_SETTINGS_LIGHTING_CONTRAST_MEDIUM) },
    { LIGHTING_CONTRAST_HIGH, GS_ID(GRAPHIC_SETTINGS_LIGHTING_CONTRAST_HIGH) },
    { -1, nullptr },
};

static const UI_SETTINGS_ENUM_ENTRY m_RenderModeEnumEntries[] = {
    { RM_SOFTWARE, GS_ID(GRAPHIC_SETTINGS_RENDER_MODE_SOFTWARE) },
    { RM_HARDWARE, GS_ID(GRAPHIC_SETTINGS_RENDER_MODE_HARDWARE) },
    { -1, nullptr },
};

static const UI_SETTINGS_ENUM_ENTRY m_AspectModeEnumEntries[] = {
    { AM_4_3, GS_ID(GRAPHIC_SETTINGS_ASPECT_MODE_4_3) },
    { AM_16_9, GS_ID(GRAPHIC_SETTINGS_ASPECT_MODE_16_9) },
    { AM_ANY, GS_ID(GRAPHIC_SETTINGS_ASPECT_MODE_ANY) },
    { -1, nullptr },
};

#endif

static const UI_SETTINGS_ENUM_ENTRY m_ScreenshotFormatEnumEntries[] = {
    { SCREENSHOT_FORMAT_JPEG, GS_ID(GRAPHIC_SETTINGS_SCREENSHOT_FORMAT_JPG) },
    { SCREENSHOT_FORMAT_PNG, GS_ID(GRAPHIC_SETTINGS_SCREENSHOT_FORMAT_PNG) },
    { -1, nullptr },
};

#if TR_VERSION == 1
// TODO: tidy me once we decide what to do about screen.c
extern int32_t Screen_GetResWidth(void);
extern int32_t Screen_GetResHeight(void);
extern bool Screen_CanSetPrevRes(void);
extern bool Screen_CanSetNextRes(void);
extern bool Screen_SetPrevRes(void);
extern bool Screen_SetNextRes(void);

static const char *M_ScreenResolution_FormatValue(
    const UI_SETTINGS_OPTION *option);
static bool M_ScreenResolution_CanChangeValue(
    const UI_SETTINGS_OPTION *option, int32_t dir);
static bool M_ScreenResolution_RequestChangeValue(
    const UI_SETTINGS_OPTION *option, int32_t dir);

static const char *M_ScreenResolution_FormatValue(
    const UI_SETTINGS_OPTION *const option)
{
    return String_FormatStatic(
        "%dx%d", Screen_GetResWidth(), Screen_GetResHeight());
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
#endif

// Custom handlers for changing UI language
static VECTOR *m_Languages = nullptr;

static void M_Language_Cleanup(void);
static const VECTOR *M_Language_GetLanguages(void);
static int32_t M_Language_FindIndex(const UI_SETTINGS_OPTION *option);
static bool M_Language_CanChangeValue(
    const UI_SETTINGS_OPTION *option, int32_t dir);
static bool M_Language_RequestChangeValue(
    const UI_SETTINGS_OPTION *option, int32_t dir);

static void M_Language_Cleanup(void)
{
    // Free the languages vector and its strings.
    if (m_Languages != nullptr) {
        for (int32_t i = 0; i < m_Languages->count; i++) {
            char *lang = *(char **)Vector_Get(m_Languages, i);
            Memory_Free(lang);
        }
        Vector_Free(m_Languages);
        m_Languages = nullptr;
    }
}

static const VECTOR *M_Language_GetLanguages(void)
{
    if (m_Languages == nullptr) {
        // Initialize available languages for the language option.
        m_Languages = GameStringManager_GetAvailableLanguages();
        atexit(M_Language_Cleanup);
    }
    return m_Languages;
}

static int32_t M_Language_FindIndex(const UI_SETTINGS_OPTION *const option)
{
    const VECTOR *const langs = M_Language_GetLanguages();
    const char *const cur = *(char **)option->target;
    for (int32_t i = 0; i < langs->count; i++) {
        const char *const lang = *(char **)Vector_Get(langs, i);
        if (String_Equivalent(lang, cur)) {
            return i;
        }
    }
    return -1;
}

static const char *M_Language_FormatValue(
    const UI_SETTINGS_OPTION *const option)
{
    const char *const code = *(const char **)option->target;
    const char *const name = GameStringManager_GetLanguageName(code);
    return name != nullptr ? name : code;
}

static bool M_Language_CanChangeValue(
    const UI_SETTINGS_OPTION *const option, const int32_t dir)
{
    const VECTOR *const langs = M_Language_GetLanguages();
    if (langs->count < 2) {
        return false;
    }
    const int32_t idx = M_Language_FindIndex(option);
    if (idx < 0) {
        return false;
    }
    return idx + dir >= 0 && idx + dir < langs->count;
}

static bool M_Language_RequestChangeValue(
    const UI_SETTINGS_OPTION *const option, const int32_t dir)
{
    const VECTOR *const langs = M_Language_GetLanguages();
    if (!M_Language_CanChangeValue(option, dir)) {
        return false;
    }
    int32_t idx = M_Language_FindIndex(option);
    const char *const new_lang = *(char **)Vector_Get(langs, idx + dir);
    Config_SetOptionValueFromString(Config_GetOption(option->target), new_lang);
    GameStringManager_ReloadLanguage(new_lang);
    return true;
}

static const UI_SETTINGS_OPTION m_VisualsOptions[] = {
    {
        .option_type = COT_INT32,
        .label_id = GS_ID(GRAPHIC_SETTINGS_FOG_START),
        .description_id = GS_ID(GRAPHIC_SETTINGS_FOG_START_DESCRIPTION),
        .target = &g_Config.visuals.fog_start,
        .min_value = 1,
        .max_value = 100,
    },

    {
        .option_type = COT_INT32,
        .label_id = GS_ID(GRAPHIC_SETTINGS_FOG_END),
        .description_id = GS_ID(GRAPHIC_SETTINGS_FOG_END_DESCRIPTION),
        .target = &g_Config.visuals.fog_end,
        .min_value = 1,
        .max_value = 100,
    },

    {
        .option_type = COT_RGB888,
        .label_id = GS_ID(GRAPHIC_SETTINGS_WATER_COLOR_R),
        .description_id = nullptr,
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
        .description_id = nullptr,
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
        .description_id = nullptr,
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
        .description_id = GS_ID(GRAPHIC_SETTINGS_CAMERA_MODE_DESCRIPTION),
        .target = &g_Config.visuals.camera_mode,
        .delta_slow = 1,
        .delta_fast = 1,
        .misc = m_CameraModeEnumEntries,
    },

#if TR_VERSION == 1
    {
        .option_type = COT_INT32,
        .label_id = GS_ID(GRAPHIC_SETTINGS_FOV),
        .description_id = GS_ID(GRAPHIC_SETTINGS_FOV_DESCRIPTION),
        .target = &g_Config.visuals.fov_value,
        .min_value = 30,
        .max_value = 150,
        .delta_slow = 1,
        .delta_fast = 10,
    },

    {
        .target = &g_Config.visuals.fov_vertical,
        .label_id = GS_ID(GRAPHIC_SETTINGS_FOV_VERTICAL),
        .description_id = GS_ID(GRAPHIC_SETTINGS_FOV_VERTICAL_DESCRIPTION),
        .option_type = COT_BOOL,
    },
#elif TR_VERSION == 2
    {
        .option_type = COT_INT32,
        .label_id = GS_ID(GRAPHIC_SETTINGS_FOV),
        .description_id = GS_ID(GRAPHIC_SETTINGS_FOV_DESCRIPTION),
        .target = &g_Config.visuals.fov,
        .min_value = 30,
        .max_value = 150,
        .delta_slow = 1,
        .delta_fast = 10,
    },

    {
        .option_type = COT_BOOL,
        .label_id = GS_ID(GRAPHIC_SETTINGS_USE_PSX_FOV),
        .description_id = GS_ID(GRAPHIC_SETTINGS_USE_PSX_FOV_DESCRIPTION),
        .target = &g_Config.visuals.use_psx_fov,
    },
#endif

#if TR_VERSION == 1
    {
        .option_type = COT_BOOL,
        .label_id = GS_ID(GRAPHIC_SETTINGS_REFLECTIONS),
        .description_id = GS_ID(GRAPHIC_SETTINGS_REFLECTIONS_DESCRIPTION),
        .target = &g_Config.visuals.enable_reflections,
    },

    {
        .target = &g_Config.visuals.enable_skybox,
        .label_id = GS_ID(GRAPHIC_SETTINGS_ENABLE_SKYBOX),
        .description_id = GS_ID(GRAPHIC_SETTINGS_ENABLE_SKYBOX_DESCRIPTION),
        .option_type = COT_BOOL,
    },
#endif

    {
        .target = &g_Config.visuals.enable_braid,
        .label_id = GS_ID(GRAPHIC_SETTINGS_ENABLE_BRAID),
        .description_id = GS_ID(GRAPHIC_SETTINGS_ENABLE_BRAID_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.visuals.enable_breeze,
        .label_id = GS_ID(GRAPHIC_SETTINGS_ENABLE_BREEZE),
        .description_id = GS_ID(GRAPHIC_SETTINGS_ENABLE_BREEZE_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.visuals.enable_3d_pickups,
        .label_id = GS_ID(GRAPHIC_SETTINGS_ENABLE_3D_PICKUPS),
        .description_id = GS_ID(GRAPHIC_SETTINGS_ENABLE_3D_PICKUPS_DESCRIPTION),
        .option_type = COT_BOOL,
    },

#if TR_VERSION == 1
    {
        .target = &g_Config.gameplay.enable_pickup_aids,
        .label_id = GS_ID(GRAPHIC_SETTINGS_ENABLE_PICKUP_AIDS),
        .description_id =
            GS_ID(GRAPHIC_SETTINGS_ENABLE_PICKUP_AIDS_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.visuals.enable_ps1_crystals,
        .label_id = GS_ID(GRAPHIC_SETTINGS_ENABLE_PS1_CRYSTALS),
        .description_id =
            GS_ID(GRAPHIC_SETTINGS_ENABLE_PS1_CRYSTALS_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.visuals.enable_round_shadow,
        .label_id = GS_ID(GRAPHIC_SETTINGS_ENABLE_ROUND_SHADOW),
        .description_id =
            GS_ID(GRAPHIC_SETTINGS_ENABLE_ROUND_SHADOW_DESCRIPTION),
        .option_type = COT_BOOL,
    },
#endif

    {
        .target = &g_Config.visuals.enable_gun_lighting,
        .label_id = GS_ID(GRAPHIC_SETTINGS_ENABLE_GUN_LIGHTING),
        .description_id =
            GS_ID(GRAPHIC_SETTINGS_ENABLE_GUN_LIGHTING_DESCRIPTION),
        .option_type = COT_BOOL,
    },

#if TR_VERSION == 1
    {
        .target = &g_Config.visuals.enable_shotgun_flash,
        .label_id = GS_ID(GRAPHIC_SETTINGS_ENABLE_SHOTGUN_FLASH),
        .description_id =
            GS_ID(GRAPHIC_SETTINGS_ENABLE_SHOTGUN_FLASH_DESCRIPTION),
        .option_type = COT_BOOL,
    },
#endif

    {
        .target = nullptr,
    },
};

static UI_SETTINGS_OPTION m_UIOptions[] = {
    {
        .option_type = COT_STRING,
        .label_id = GS_ID(GRAPHIC_SETTINGS_UI_LANGUAGE),
        .description_id = GS_ID(GRAPHIC_SETTINGS_UI_LANGUAGE_DESCRIPTION),
        .target = &g_Config.language,
        .custom_handler = {
            .format_value = M_Language_FormatValue,
            .can_change_value = M_Language_CanChangeValue,
            .request_change_value = M_Language_RequestChangeValue,
        },
    },

    {
        .option_type = COT_DOUBLE,
        .label_id = GS_ID(GRAPHIC_SETTINGS_UI_TEXT_SCALE),
        .description_id = GS_ID(GRAPHIC_SETTINGS_UI_TEXT_SCALE_DESCRIPTION),
        .target = &g_Config.ui.text_scale,
        .min_value = 50,
        .max_value = 200,
        .delta_slow = 10,
        .delta_fast = 10,
    },

    {
        .option_type = COT_DOUBLE,
        .label_id = GS_ID(GRAPHIC_SETTINGS_UI_BAR_SCALE),
        .description_id = GS_ID(GRAPHIC_SETTINGS_UI_BAR_SCALE_DESCRIPTION),
        .target = &g_Config.ui.bar_scale,
        .min_value = 50,
        .max_value = 200,
        .delta_slow = 10,
        .delta_fast = 10,
    },

    {
        .option_type = COT_BOOL,
        .label_id = GS_ID(GRAPHIC_SETTINGS_UI_SCROLL_WRAPAROUND),
        .description_id =
            GS_ID(GRAPHIC_SETTINGS_UI_SCROLL_WRAPAROUND_DESCRIPTION),
        .target = &g_Config.ui.enable_wraparound,
    },

#if TR_VERSION == 1
    {
        .target = &g_Config.ui.menu_style,
        .label_id = GS_ID(GRAPHIC_SETTINGS_MENU_STYLE),
        .description_id = GS_ID(GRAPHIC_SETTINGS_MENU_STYLE_DESCRIPTION),
        .option_type = COT_ENUM,
        .misc = &m_MenuStyleEnumEntries,
    },
#endif

    {
        .target = &g_Config.ui.lara_health_bar.show_mode,
        .label_id = GS_ID(GRAPHIC_SETTINGS_HEALTHBAR_SHOW_MODE),
        .description_id =
            GS_ID(GRAPHIC_SETTINGS_HEALTHBAR_SHOW_MODE_DESCRIPTION),
        .option_type = COT_ENUM,
        .misc = &m_HealthBarShowModeEnumEntries,
    },
    {
        .target = &g_Config.ui.lara_health_bar.location,
        .label_id = GS_ID(GRAPHIC_SETTINGS_HEALTHBAR_LOCATION),
        .description_id =
            GS_ID(GRAPHIC_SETTINGS_HEALTHBAR_LOCATION_DESCRIPTION),
        .option_type = COT_ENUM,
        .misc = &m_BarLocationEnumEntries,
    },
    {
        .target = &g_Config.ui.lara_health_bar.color,
        .label_id = GS_ID(GRAPHIC_SETTINGS_HEALTHBAR_COLOR),
        .description_id = GS_ID(GRAPHIC_SETTINGS_HEALTHBAR_COLOR_DESCRIPTION),
        .option_type = COT_ENUM,
        .misc = &m_BarColorEnumEntries,
    },

    {
        .target = &g_Config.ui.lara_air_bar.show_mode,
        .label_id = GS_ID(GRAPHIC_SETTINGS_AIRBAR_SHOW_MODE),
        .description_id = GS_ID(GRAPHIC_SETTINGS_AIRBAR_SHOW_MODE_DESCRIPTION),
        .option_type = COT_ENUM,
        .misc = &m_AirBarShowModeEnumEntries,
    },
    {
        .target = &g_Config.ui.lara_air_bar.location,
        .label_id = GS_ID(GRAPHIC_SETTINGS_AIRBAR_LOCATION),
        .description_id = GS_ID(GRAPHIC_SETTINGS_AIRBAR_LOCATION_DESCRIPTION),
        .option_type = COT_ENUM,
        .misc = &m_BarLocationEnumEntries,
    },
    {
        .target = &g_Config.ui.lara_air_bar.color,
        .label_id = GS_ID(GRAPHIC_SETTINGS_AIRBAR_COLOR),
        .description_id = GS_ID(GRAPHIC_SETTINGS_AIRBAR_COLOR_DESCRIPTION),
        .option_type = COT_ENUM,
        .misc = &m_BarColorEnumEntries,
    },

    {
        .target = &g_Config.ui.enemy_health_bar.show_mode,
        .label_id = GS_ID(GRAPHIC_SETTINGS_ENEMY_HEALTHBAR_SHOW_MODE),
        .description_id =
            GS_ID(GRAPHIC_SETTINGS_ENEMY_HEALTHBAR_SHOW_MODE_DESCRIPTION),
        .option_type = COT_ENUM,
        .misc = &m_EnemyHealthBarShowModeEnumEntries,
    },
    {
        .target = &g_Config.ui.enemy_health_bar.location,
        .label_id = GS_ID(GRAPHIC_SETTINGS_ENEMY_HEALTHBAR_LOCATION),
        .description_id =
            GS_ID(GRAPHIC_SETTINGS_ENEMY_HEALTHBAR_LOCATION_DESCRIPTION),
        .option_type = COT_ENUM,
        .misc = &m_BarLocationEnumEntries,
    },
    {
        .target = &g_Config.ui.enemy_health_bar.color,
        .label_id = GS_ID(GRAPHIC_SETTINGS_ENEMY_HEALTHBAR_COLOR),
        .description_id =
            GS_ID(GRAPHIC_SETTINGS_ENEMY_HEALTHBAR_COLOR_DESCRIPTION),
        .option_type = COT_ENUM,
        .misc = &m_BarColorEnumEntries,
    },

#if TR_VERSION == 1
    {
        .target = &g_Config.ui.enable_smooth_bars,
        .label_id = GS_ID(GRAPHIC_SETTINGS_ENABLE_SMOOTH_BARS),
        .description_id =
            GS_ID(GRAPHIC_SETTINGS_ENABLE_SMOOTH_BARS_DESCRIPTION),
        .option_type = COT_BOOL,
    },
#endif

    {
        .target = &g_Config.visuals.enable_fade_effects,
        .label_id = GS_ID(GRAPHIC_SETTINGS_ENABLE_FADE_EFFECTS),
        .description_id =
            GS_ID(GRAPHIC_SETTINGS_ENABLE_FADE_EFFECTS_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = &g_Config.visuals.enable_exit_fade_effects,
        .label_id = GS_ID(GRAPHIC_SETTINGS_ENABLE_EXIT_FADE_EFFECTS),
        .description_id =
            GS_ID(GRAPHIC_SETTINGS_ENABLE_EXIT_FADE_EFFECTS_DESCRIPTION),
        .option_type = COT_BOOL,
    },

    {
        .target = nullptr,
    },
};

static const UI_SETTINGS_OPTION m_RenderOptions[] = {
    {
        .option_type = COT_INT32,
        .label_id = GS_ID(GRAPHIC_SETTINGS_FPS),
        .description_id = GS_ID(GRAPHIC_SETTINGS_FPS_DESCRIPTION),
        .target = &g_Config.rendering.fps,
        .min_value = 30,
        .max_value = 60,
        .delta_slow = 30,
        .delta_fast = 30,
    },

    {
        .option_type = COT_ENUM,
        .label_id = GS_ID(GRAPHIC_SETTINGS_TEXTURE_FILTER),
        .description_id = GS_ID(GRAPHIC_SETTINGS_TEXTURE_FILTER_DESCRIPTION),
        .target = &g_Config.rendering.texture_filter,
        .delta_slow = 1,
        .delta_fast = 1,
        .misc = m_TextureFilterEnumEntries,
    },

    {
        .option_type = COT_BOOL,
        .label_id = GS_ID(GRAPHIC_SETTINGS_TRAPEZOID_FILTER),
        .description_id = GS_ID(GRAPHIC_SETTINGS_TRAPEZOID_FILTER_DESCRIPTION),
        .target = &g_Config.rendering.enable_trapezoid_filter,
    },

#if TR_VERSION == 1

    {
        .target = &g_Config.rendering.anisotropy_filter,
        .label_id = GS_ID(GRAPHIC_SETTINGS_ANISOTROPY_FILTER),
        .description_id = GS_ID(GRAPHIC_SETTINGS_ANISOTROPY_FILTER_DESCRIPTION),
        .option_type = COT_FLOAT,
        .min_value = 1 * 100,
        .max_value = 32 * 100,
        .delta_slow = 10,
        .delta_fast = 100,
    },

    {
        .option_type = COT_ENUM,
        .label_id = GS_ID(GRAPHIC_SETTINGS_FBO_FILTER),
        .description_id = GS_ID(GRAPHIC_SETTINGS_FBO_FILTER_DESCRIPTION),
        .target = &g_Config.rendering.fbo_filter,
        .delta_slow = 1,
        .delta_fast = 1,
        .misc = m_TextureFilterEnumEntries,
    },

    {
        .label_id = GS_ID(GRAPHIC_SETTINGS_RESOLUTION),
        .description_id = GS_ID(GRAPHIC_SETTINGS_RESOLUTION_DESCRIPTION),
        .custom_handler = {
            .format_value = M_ScreenResolution_FormatValue,
            .can_change_value = M_ScreenResolution_CanChangeValue,
            .request_change_value = M_ScreenResolution_RequestChangeValue,
        },
    },

    {
        .option_type = COT_ENUM,
        .label_id = GS_ID(GRAPHIC_SETTINGS_RENDER_MODE),
        .description_id = GS_ID(GRAPHIC_SETTINGS_RENDER_MODE_DESCRIPTION),
        .target = &g_Config.rendering.render_mode,
        .delta_slow = 1,
        .delta_fast = 1,
        .misc = m_RenderModeEnumEntries,
    },

    {
        .option_type = COT_BOOL,
        .label_id = GS_ID(GRAPHIC_SETTINGS_VSYNC),
        .description_id = GS_ID(GRAPHIC_SETTINGS_VSYNC_DESCRIPTION),
        .target = &g_Config.rendering.enable_vsync,
    },

    {
        .option_type = COT_FLOAT,
        .label_id = GS_ID(GRAPHIC_SETTINGS_BRIGHTNESS),
        .description_id = GS_ID(GRAPHIC_SETTINGS_BRIGHTNESS_DESCRIPTION),
        .target = &g_Config.visuals.brightness,
        .min_value = CONFIG_MIN_BRIGHTNESS * 100,
        .max_value = CONFIG_MAX_BRIGHTNESS * 100,
        .delta_slow = 10,
        .delta_fast = 10,
    },

#elif TR_VERSION == 2
    {
        .option_type = COT_BOOL,
        .label_id = GS_ID(GRAPHIC_SETTINGS_DEPTH_BUFFER),
        .description_id = GS_ID(GRAPHIC_SETTINGS_DEPTH_BUFFER_DESCRIPTION),
        .target = &g_Config.rendering.enable_zbuffer,
    },

    {
        .option_type = COT_ENUM,
        .label_id = GS_ID(GRAPHIC_SETTINGS_LIGHTING_CONTRAST),
        .description_id = GS_ID(GRAPHIC_SETTINGS_LIGHTING_CONTRAST_DESCRIPTION),
        .target = &g_Config.rendering.lighting_contrast,
        .delta_slow = 1,
        .delta_fast = 1,
        .misc = m_LightingContrastEnumEntries,
    },

    {
        .option_type = COT_ENUM,
        .label_id = GS_ID(GRAPHIC_SETTINGS_RENDER_MODE),
        .description_id = GS_ID(GRAPHIC_SETTINGS_RENDER_MODE_DESCRIPTION),
        .target = &g_Config.rendering.render_mode,
        .delta_slow = 1,
        .delta_fast = 1,
        .misc = m_RenderModeEnumEntries,
    },

    {
        .option_type = COT_ENUM,
        .label_id = GS_ID(GRAPHIC_SETTINGS_ASPECT_MODE),
        .description_id = GS_ID(GRAPHIC_SETTINGS_ASPECT_MODE_DESCRIPTION),
        .target = &g_Config.rendering.aspect_mode,
        .delta_slow = 1,
        .delta_fast = 1,
        .misc = m_AspectModeEnumEntries,
    },

    {
        .option_type = COT_INT32,
        .label_id = GS_ID(GRAPHIC_SETTINGS_SCALER),
        .description_id = GS_ID(GRAPHIC_SETTINGS_SCALER_DESCRIPTION),
        .target = &g_Config.rendering.scaler,
        .min_value = 1,
        .max_value = 4,
        .delta_slow = 1,
        .delta_fast = 1,
    },

    {
        .option_type = COT_FLOAT,
        .label_id = GS_ID(GRAPHIC_SETTINGS_SIZER),
        .description_id = GS_ID(GRAPHIC_SETTINGS_SIZER_DESCRIPTION),
        .target = &g_Config.rendering.sizer,
        .min_value = 40,
        .max_value = 100,
        .delta_slow = 5,
        .delta_fast = 5,
    },
#endif

    {
        .target = &g_Config.rendering.screenshot_format,
        .label_id = GS_ID(GRAPHIC_SETTINGS_SCREENSHOT_FORMAT),
        .description_id = GS_ID(GRAPHIC_SETTINGS_SCREENSHOT_FORMAT_DESCRIPTION),
        .option_type = COT_ENUM,
        .misc = &m_ScreenshotFormatEnumEntries,
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
