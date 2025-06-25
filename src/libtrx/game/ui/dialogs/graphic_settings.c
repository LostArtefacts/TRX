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
    { GFX_TF_NN, GS_ID(GRAPHIC_SETTINGS_NEAREST_NEIGHBOR) },
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
    bool result = false;
    if (dir < 0) {
        result = Screen_SetPrevRes();
    } else {
        result = Screen_SetNextRes();
    }
    if (result) {
        g_Config.rendering.resolution_width = Screen_GetResWidth();
        g_Config.rendering.resolution_height = Screen_GetResHeight();
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
    const int32_t idx = M_Language_FindIndex(option);
    if (idx < 0) {
        // If the language from the user config somehow is no longer on the list
        // (the file was deleted), let the player return to the default language
        return true;
    }
    if (langs->count < 2) {
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
    const char *new_lang;
    const int32_t idx = M_Language_FindIndex(option);
    if (idx != -1) {
        new_lang = *(char **)Vector_Get(langs, idx + dir);
    } else {
        // If the language from the user config somehow is no longer on the list
        // (the file was deleted), default to the first entry, which is English
        new_lang = *(char **)Vector_Get(langs, 0);
    }
    Config_SetOptionValueFromString(Config_GetOption(option->target), new_lang);
    GameStringManager_ReloadLanguage(new_lang);
    return true;
}

// Custom handlers for graying out dependent visual settings
static bool M_EnableBreeze_IsAvailable(const UI_SETTINGS_OPTION *option);
#if TR_VERISON == 1
static bool M_EnablePS1Crystals_IsAvailable(const UI_SETTINGS_OPTION *option);
#endif
static bool M_Healthbar_IsAvailable(const UI_SETTINGS_OPTION *option);
static bool M_Airbar_IsAvailable(const UI_SETTINGS_OPTION *option);
static bool M_EnemyHealthbar_IsAvailable(const UI_SETTINGS_OPTION *option);
static bool M_EnableExitFadeEffects_IsAvailable(
    const UI_SETTINGS_OPTION *option);

static bool M_EnableBreeze_IsAvailable(const UI_SETTINGS_OPTION *const option)
{
    return g_Config.visuals.enable_braid;
}

#if TR_VERSION == 1
static bool M_EnablePS1Crystals_IsAvailable(
    const UI_SETTINGS_OPTION *const option)
{
    return g_Config.gameplay.enable_save_crystals;
}
#endif

static bool M_Healthbar_IsAvailable(const UI_SETTINGS_OPTION *const option)
{
    return g_Config.ui.lara_health_bar.show_mode != BSM_NEVER;
}

static bool M_Airbar_IsAvailable(const UI_SETTINGS_OPTION *const option)
{
    return g_Config.ui.lara_air_bar.show_mode != BSM_NEVER;
}

static bool M_EnemyHealthbar_IsAvailable(const UI_SETTINGS_OPTION *const option)
{
    return g_Config.ui.enemy_health_bar.show_mode != BSM_NEVER;
}

static bool M_EnableExitFadeEffects_IsAvailable(
    const UI_SETTINGS_OPTION *const option)
{
    return g_Config.visuals.enable_fade_effects;
}

static const UI_SETTINGS_OPTION m_VisualsOptions[] = {
#include "setting_tabs/graphic_visuals.def"
    { .target = nullptr },
};

static UI_SETTINGS_OPTION m_UIOptions[] = {
#include "setting_tabs/graphic_ui.def"
    { .target = nullptr },
};

static const UI_SETTINGS_OPTION m_RenderOptions[] = {
#include "setting_tabs/graphic_rendering.def"
    { .target = nullptr },
};

static const UI_SETTINGS_TAB m_Tabs[] = {
    { GS_ID(GRAPHIC_SETTINGS_VISUALS_TAB), m_VisualsOptions },
    { GS_ID(GRAPHIC_SETTINGS_UI_TAB), m_UIOptions },
    { GS_ID(GRAPHIC_SETTINGS_RENDERING_TAB), m_RenderOptions },
};

UI_SETTINGS_STATE *UI_GraphicSettings_Init(void)
{
    return UI_Settings_InitWithTabs(
        GS_ID(GRAPHIC_SETTINGS_TITLE), sizeof(m_Tabs) / sizeof(m_Tabs[0]),
        m_Tabs);
}

void UI_GraphicSettings_Free(UI_SETTINGS_STATE *const s)
{
    UI_Settings_Free(s);
}

bool UI_GraphicSettings_Control(UI_SETTINGS_STATE *const s)
{
    return UI_Settings_Control(s);
}

void UI_GraphicSettings(UI_SETTINGS_STATE *const s)
{
    UI_Settings(s);
}
