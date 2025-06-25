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
    {
        .option_type = COT_FLOAT_PERCENT,
        .label_id = GS_ID(SOUND_SETTINGS_SOUND_VOLUME),
        .description_id = GS_ID(SOUND_SETTINGS_SOUND_VOLUME_DESCRIPTION),
        .custom_handler = {
            .format_value = nullptr,
            .can_change_value = nullptr,
            .request_change_value = M_Volume_RequestChange,
        },
        .target = &g_Config.audio.sound_volume,
        .min_value = 0,
        .max_value = 100,
        .delta_slow = 10,
        .delta_fast = 10,
    },
    {
        .option_type = COT_FLOAT_PERCENT,
        .label_id = GS_ID(SOUND_SETTINGS_MUSIC_VOLUME),
        .description_id = GS_ID(SOUND_SETTINGS_MUSIC_VOLUME_DESCRIPTION),
        .custom_handler = {
            .format_value = nullptr,
            .can_change_value = nullptr,
            .request_change_value = M_Volume_RequestChange,
        },
        .target = &g_Config.audio.music_volume,
        .min_value = 0,
        .max_value = 100,
        .delta_slow = 10,
        .delta_fast = 10,
    },
#if TR_VERSION == 1
    {
        .target = &g_Config.audio.enable_music_in_menu,
        .label_id = GS_ID(SOUND_SETTINGS_ENABLE_MUSIC_IN_MENU),
        .description_id = GS_ID(SOUND_SETTINGS_ENABLE_MUSIC_IN_MENU_DESCRIPTION),
        .option_type = COT_BOOL,
    },
    {
        .target = &g_Config.audio.fix_secrets_killing_music,
        .label_id = GS_ID(SOUND_SETTINGS_FIX_SECRETS_KILLING_MUSIC),
        .description_id = GS_ID(SOUND_SETTINGS_FIX_SECRETS_KILLING_MUSIC_DESCRIPTION),
        .option_type = COT_BOOL,
    },
    {
        .target = &g_Config.audio.fix_speeches_killing_music,
        .label_id = GS_ID(SOUND_SETTINGS_FIX_SPEECHES_KILLING_MUSIC),
        .description_id = GS_ID(SOUND_SETTINGS_FIX_SPEECHES_KILLING_MUSIC_DESCRIPTION),
        .option_type = COT_BOOL,
    },
#endif
    {
        .target = &g_Config.audio.enable_music_in_inventory,
        .label_id = GS_ID(SOUND_SETTINGS_PAUSE_MUSIC_IN_INVENTORY),
        .description_id = GS_ID(SOUND_SETTINGS_PAUSE_MUSIC_IN_INVENTORY_DESCRIPTION),
        .option_type = COT_INVERTED_BOOL,
    },
    {
        .target = &g_Config.audio.inventory_ambient_volume,
        .label_id = GS_ID(SOUND_SETTINGS_INVENTORY_AMBIENT_VOLUME),
        .description_id = GS_ID(SOUND_SETTINGS_INVENTORY_AMBIENT_VOLUME_DESCRIPTION),
        .option_type = COT_FLOAT_PERCENT,
        .min_value = 0,
        .max_value = 100,
        .delta_slow = 10,
        .delta_fast = 10,
        .custom_handler = {
            .format_value = nullptr,
            .can_change_value = nullptr,
            .request_change_value = nullptr,
            .is_available = M_PauseMusicInInventory_IsAvailable,
        },
    },
    {
        .target = &g_Config.audio.inventory_music_volume,
        .label_id = GS_ID(SOUND_SETTINGS_INVENTORY_MUSIC_VOLUME),
        .description_id = GS_ID(SOUND_SETTINGS_INVENTORY_MUSIC_VOLUME_DESCRIPTION),
        .option_type = COT_FLOAT_PERCENT,
        .min_value = 0,
        .max_value = 100,
        .delta_slow = 10,
        .delta_fast = 10,
        .custom_handler = {
            .format_value = nullptr,
            .can_change_value = nullptr,
            .request_change_value = nullptr,
            .is_available = M_PauseMusicInInventory_IsAvailable,
        },
    },
    {
        .target = &g_Config.audio.underwater_ambient_volume,
        .label_id = GS_ID(SOUND_SETTINGS_UNDERWATER_AMBIENT_VOLUME),
        .description_id = GS_ID(SOUND_SETTINGS_UNDERWATER_AMBIENT_VOLUME_DESCRIPTION),
        .option_type = COT_FLOAT_PERCENT,
        .min_value = 0,
        .max_value = 100,
        .delta_slow = 10,
        .delta_fast = 10,
        .custom_handler = {
            .format_value = nullptr,
            .can_change_value = nullptr,
            .request_change_value = nullptr,
        },
    },
    {
        .target = &g_Config.audio.underwater_music_volume,
        .label_id = GS_ID(SOUND_SETTINGS_UNDERWATER_MUSIC_VOLUME),
        .description_id = GS_ID(SOUND_SETTINGS_UNDERWATER_MUSIC_VOLUME_DESCRIPTION),
        .option_type = COT_FLOAT_PERCENT,
        .min_value = 0,
        .max_value = 100,
        .delta_slow = 10,
        .delta_fast = 10,
        .custom_handler = {
            .format_value = nullptr,
            .can_change_value = nullptr,
            .request_change_value = nullptr,
        },
    },
    {
        .target = &g_Config.audio.music_load_condition,
        .label_id = GS_ID(SOUND_SETTINGS_MUSIC_LOAD_CONDITION),
        .description_id = GS_ID(SOUND_SETTINGS_MUSIC_LOAD_CONDITION_DESCRIPTION),
        .option_type = COT_ENUM,
        .misc = &m_MusicLoadConditionEnumEntries,
    },
#if TR_VERSION == 1
    {
        .target = &g_Config.audio.enable_pitched_sounds,
        .label_id = GS_ID(SOUND_SETTINGS_ENABLE_PITCHED_SOUNDS),
        .description_id = GS_ID(SOUND_SETTINGS_ENABLE_PITCHED_SOUNDS_DESCRIPTION),
        .option_type = COT_BOOL,
    },
    {
        .target = &g_Config.audio.enable_ps_uzi_sfx,
        .label_id = GS_ID(SOUND_SETTINGS_PS_UZI_SFX),
        .description_id = GS_ID(SOUND_SETTINGS_PS_UZI_SFX_DESCRIPTION),
        .option_type = COT_BOOL,
    },
#elif TR_VERSION >= 2
    {
        .target = &g_Config.audio.enable_lara_mic,
        .label_id = GS_ID(SOUND_SETTINGS_ENABLE_LARA_MIC),
        .description_id = GS_ID(SOUND_SETTINGS_ENABLE_LARA_MIC_DESCRIPTION),
        .option_type = COT_BOOL,
    },
#endif
    {
        .target = nullptr,
    },
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
