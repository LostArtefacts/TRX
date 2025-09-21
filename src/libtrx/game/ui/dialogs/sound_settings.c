#include "game/ui/dialogs/sound_settings.h"

#include "config.h"
#include "game/lara/const.h"
#include "game/ui/dialogs/setting_helpers/enums.h"
#include "game/ui/dialogs/setting_helpers/handlers.h"

static const UI_SETTINGS_OPTION m_SoundVolumeOptions[] = {
#include "setting_tabs/sound_volume.def"
    { .target = nullptr },
};

static const UI_SETTINGS_OPTION m_SoundMiscOptions[] = {
#include "setting_tabs/sound_misc.def"
    { .target = nullptr },
};

static const UI_SETTINGS_TAB m_Tabs[] = {
    { GS_ID(SOUND_SETTINGS_VOLUME_TAB), m_SoundVolumeOptions },
    { GS_ID(SOUND_SETTINGS_MISC_TAB), m_SoundMiscOptions },
};

UI_SETTINGS_STATE *UI_SoundSettings_Init(void)
{
    return UI_Settings_InitWithTabs(
        GS_ID(SOUND_SETTINGS_TITLE), sizeof(m_Tabs) / sizeof(m_Tabs[0]),
        m_Tabs);
}

void UI_SoundSettings_Free(UI_SETTINGS_STATE *const s)
{
    UI_Settings_Free(s);
}

bool UI_SoundSettings_Control(UI_SETTINGS_STATE *const s)
{
    return UI_Settings_Control(s);
}

void UI_SoundSettings(UI_SETTINGS_STATE *const s)
{
    UI_Settings(s);
}
