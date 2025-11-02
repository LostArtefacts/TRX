#include <trx/game/ui/dialogs/graphic_settings.h>

#include <trx/config.h>
#include <trx/game/ui/dialogs/setting_helpers/enums.h>
#include <trx/game/ui/dialogs/setting_helpers/handlers.h>

static const UI_SETTINGS_OPTION m_VisualsOptions[] = {
#include <trx/game/ui/dialogs/setting_tabs/graphic_visuals.def>
    { .target = nullptr },
};

static UI_SETTINGS_OPTION m_UIOptions[] = {
#include <trx/game/ui/dialogs/setting_tabs/graphic_ui.def>
    { .target = nullptr },
};

static const UI_SETTINGS_OPTION m_UIBarsOptions[] = {
#include <trx/game/ui/dialogs/setting_tabs/graphic_ui_bars.def>
    { .target = nullptr },
};

static const UI_SETTINGS_OPTION m_RenderOptions[] = {
#include <trx/game/ui/dialogs/setting_tabs/graphic_rendering.def>
    { .target = nullptr },
};

static const UI_SETTINGS_TAB m_Tabs[] = {
    { GS_ID(GRAPHIC_SETTINGS_VISUALS_TAB), m_VisualsOptions },
    { GS_ID(GRAPHIC_SETTINGS_UI_TAB), m_UIOptions },
    { GS_ID(GRAPHIC_SETTINGS_UI_BARS_TAB), m_UIBarsOptions },
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
