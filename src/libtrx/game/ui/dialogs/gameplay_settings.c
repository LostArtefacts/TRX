#include "game/ui/dialogs/gameplay_settings.h"

#include "config.h"
#include "game/lara/const.h"
#include "game/ui/dialogs/setting_helpers/enums.h"

static bool M_FixItemRots_IsAvailable(const UI_SETTINGS_OPTION *option);
static bool M_FixStepGlitch_IsAvailable(const UI_SETTINGS_OPTION *option);
static bool M_FixWadeWallHit_IsAvailable(const UI_SETTINGS_OPTION *option);

static bool M_FixItemRots_IsAvailable(const UI_SETTINGS_OPTION *const option)
{
    return g_Config.visuals.enable_3d_pickups;
}

static bool M_FixStepGlitch_IsAvailable(const UI_SETTINGS_OPTION *const option)
{
    return g_Config.gameplay.enable_smooth_wall_deflect;
}

static bool M_FixWadeWallHit_IsAvailable(const UI_SETTINGS_OPTION *const option)
{
#if TR_VERSION == 1
    return g_Config.gameplay.enable_wading;
#else
    return true;
#endif
}

static const UI_SETTINGS_OPTION m_GeneralOptions[] = {
#include "setting_tabs/gameplay_general.def"
    { .target = nullptr },
};

static const UI_SETTINGS_OPTION m_ControlOptions[] = {
#include "setting_tabs/gameplay_controls.def"
    { .target = nullptr },
};

static const UI_SETTINGS_OPTION m_GameplayModOptions[] = {
#include "setting_tabs/gameplay_mods.def"
    { .target = nullptr },
};

static const UI_SETTINGS_OPTION m_GameplayFixOptions[] = {
#include "setting_tabs/gameplay_fixes.def"
    { .target = nullptr },
};

static const UI_SETTINGS_TAB m_Tabs[] = {
    { GS_ID(GAMEPLAY_SETTINGS_GENERAL_TAB), m_GeneralOptions },
    { GS_ID(GAMEPLAY_SETTINGS_CONTROLS_TAB), m_ControlOptions },
    { GS_ID(GAMEPLAY_SETTINGS_MODS_TAB), m_GameplayModOptions },
    { GS_ID(GAMEPLAY_SETTINGS_FIXES_TAB), m_GameplayFixOptions },
};

UI_SETTINGS_STATE *UI_GameplaySettings_Init(void)
{
    return UI_Settings_InitWithTabs(
        GS_ID(GAMEPLAY_SETTINGS_TITLE), sizeof(m_Tabs) / sizeof(m_Tabs[0]),
        m_Tabs);
}

void UI_GameplaySettings_Free(UI_SETTINGS_STATE *const s)
{
    UI_Settings_Free(s);
}

bool UI_GameplaySettings_Control(UI_SETTINGS_STATE *const s)
{
    return UI_Settings_Control(s);
}

void UI_GameplaySettings(UI_SETTINGS_STATE *const s)
{
    UI_Settings(s);
}
