#include "game/ui/dialogs/gameplay_settings.h"

#include "config.h"
#include "game/lara/const.h"
#include "game/ui/dialogs/setting_helpers/enums.h"
#include "game/ui/dialogs/setting_helpers/handlers.h"

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
