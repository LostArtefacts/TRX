#include <trx/game/ui/dialogs/graphic_settings.h>

#include <trx/game/ui/dialogs/settings_rows.h>
#include <trx/game/ui/dialogs/settings_tabs.h>

UI_SETTINGS_DIALOG_STATE *UI_GraphicSettings_Init(void)
{
    const UI_SETTINGS_TAB tabs[] = {
        UI_SettingsTab_MakeEditor(
            GS_ID("general/settings/graphic_settings/tabs/visuals"),
            CONFIG_TAB_GRAPHIC_VISUALS),
        UI_SettingsTab_MakeEditor(
            GS_ID("general/settings/graphic_settings/tabs/ui"),
            CONFIG_TAB_GRAPHIC_UI),
        UI_SettingsTab_MakeEditor(
            GS_ID("general/settings/graphic_settings/tabs/stats"),
            CONFIG_TAB_GRAPHIC_UI_STATS),
        UI_SettingsTab_MakeEditor(
            GS_ID("general/settings/graphic_settings/tabs/bars"),
            CONFIG_TAB_GRAPHIC_UI_BARS),
        UI_SettingsTab_MakeEditor(
            GS_ID("general/settings/graphic_settings/tabs/rendering"),
            CONFIG_TAB_GRAPHIC_RENDERING),
    };

    return UI_SettingsDialog_Init(
        GS_ID("general/settings/graphic_settings/title"), ARRAY_SIZE(tabs),
        tabs);
}

void UI_GraphicSettings_Free(UI_SETTINGS_DIALOG_STATE *const s)
{
    UI_SettingsDialog_Free(s);
}

bool UI_GraphicSettings_Control(UI_SETTINGS_DIALOG_STATE *const s)
{
    return UI_SettingsDialog_Control(s);
}

void UI_GraphicSettings(UI_SETTINGS_DIALOG_STATE *const s)
{
    UI_SettingsDialog(s);
}
