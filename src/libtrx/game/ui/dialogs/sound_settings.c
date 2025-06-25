#include "game/ui/dialogs/sound_settings.h"

#include "config.h"
#include "game/ui/dialogs/setting_helpers/enums.h"
#include "game/ui/dialogs/setting_helpers/handlers.h"

static const UI_SETTINGS_OPTION m_SoundOptions[] = {
#include "setting_tabs/sound_settings.def"
    { .target = nullptr },
};

UI_SETTINGS_STATE *UI_SoundSettings_Init(void)
{
    return UI_Settings_Init(GS_ID(SOUND_SETTINGS_TITLE), m_SoundOptions);
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
