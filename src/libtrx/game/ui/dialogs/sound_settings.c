#include "game/ui/dialogs/sound_settings.h"

#include "config.h"
#include "game/game_string.h"
#include "game/music.h"
#include "game/sound.h"
#include "memory.h"
#include "strings.h"
#include "utils.h"

static char *m_TempString = nullptr;
static size_t m_TempStringCap = 0;

static const UI_SETTINGS_ENUM_ENTRY m_MusicLoadConditionEnumEntries[] = {
    { MUSIC_LOAD_NEVER, GS_ID(SOUND_SETTINGS_MUSIC_LOAD_CONDITION_NEVER) },
    { MUSIC_LOAD_NON_AMBIENT,
      GS_ID(SOUND_SETTINGS_MUSIC_LOAD_CONDITION_NON_AMBIENT) },
    { MUSIC_LOAD_ALWAYS, GS_ID(SOUND_SETTINGS_MUSIC_LOAD_CONDITION_ALWAYS) },
    { -1, nullptr },
};

static bool M_Volume_RequestChange(
    const UI_SETTINGS_OPTION *option, int32_t dir);
static bool M_PauseMusicInInventory_IsAvailable(
    const UI_SETTINGS_OPTION *option);

static const UI_SETTINGS_OPTION m_SoundOptions[] = {
#include "setting_tabs/sound_settings.def"
    { .target = nullptr },
};

static bool M_Volume_RequestChange(
    const UI_SETTINGS_OPTION *const option, const int32_t dir)
{
    UI_Settings_RequestChange(option, dir);
    if (option->target == &g_Config.audio.music_volume) {
        Music_SetVolume(g_Config.audio.music_volume);
    } else if (option->target == &g_Config.audio.sound_volume) {
        Sound_SetMasterVolume(g_Config.audio.sound_volume);
    }
    Sound_Effect(SFX_MENU_PASSPORT, nullptr, SPM_ALWAYS);
    return true;
}

static bool M_PauseMusicInInventory_IsAvailable(
    const UI_SETTINGS_OPTION *const option)
{
    return g_Config.audio.enable_music_in_inventory;
}

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
