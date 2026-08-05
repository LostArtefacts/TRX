#include <trx/game/ui/dialogs/gameplay_settings.h>

#include <trx/game/ui/dialogs/settings_rows.h>
#include <trx/game/ui/dialogs/settings_tabs.h>

UI_SETTINGS_DIALOG_STATE *UI_GameplaySettings_Init(void)
{
    const UI_SETTINGS_TAB tabs[] = {
        UI_SettingsTab_MakeEditor(
            GS_ID("general/settings/gameplay/tabs/general"),
            CONFIG_TAB_GAMEPLAY_GENERAL),
        UI_SettingsTab_MakeEditor(
            GS_ID("general/settings/gameplay/tabs/controls"),
            CONFIG_TAB_GAMEPLAY_CONTROLS),
        UI_SettingsTab_MakeEditor(
            GS_ID("general/settings/gameplay/tabs/mods"),
            CONFIG_TAB_GAMEPLAY_MODS),
        UI_SettingsTab_MakeEditor(
            GS_ID("general/settings/gameplay/tabs/fixes"),
            CONFIG_TAB_GAMEPLAY_FIXES),
        UI_SettingsTab_MakePresets(
            GS_ID("general/settings/gameplay/tabs/presets")),
    };

    return UI_SettingsDialog_Init(
        GS_ID("general/settings/gameplay/title"), ARRAY_SIZE(tabs), tabs);
}

void UI_GameplaySettings_Free(UI_SETTINGS_DIALOG_STATE *const s)
{
    UI_SettingsDialog_Free(s);
}

bool UI_GameplaySettings_Control(UI_SETTINGS_DIALOG_STATE *const s)
{
    return UI_SettingsDialog_Control(s);
}

void UI_GameplaySettings(UI_SETTINGS_DIALOG_STATE *const s)
{
    UI_SettingsDialog(s);
}
