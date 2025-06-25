#include "game/ui/dialogs/graphic_settings.h"

#include "config.h"
#include "game/game_string_manager.h"
#include "game/ui/dialogs/setting_helpers/enums.h"
#include "memory.h"
#include "strings.h"

#include <stdlib.h>

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
