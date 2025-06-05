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

// Custom handlers for changing sound settings.
static const char *M_FormatPercentage(const UI_SETTINGS_OPTION *option);
static bool M_RequestChange(const UI_SETTINGS_OPTION *option, int32_t dir);

#define KEEP_IT_SIMPLE (TR_VERSION > 1)
static const UI_SETTINGS_OPTION m_SoundOptions[] = {
    {
        .option_type = COT_FLOAT,
        .label_id = GS_ID(SOUND_SETTINGS_SOUND_VOLUME),
        .custom_handler = {
            .format_value = M_FormatPercentage,
            .can_change_value = nullptr,
            .request_change_value = M_RequestChange,
        },
        .target = &g_Config.audio.music_volume,
        .min_value = 0,
        .max_value = 100,
        .delta_slow = 10,
        .delta_fast = 10,
    },
    {
        .option_type = COT_FLOAT,
        .label_id = GS_ID(SOUND_SETTINGS_MUSIC_VOLUME),
        .custom_handler = {
            .format_value = M_FormatPercentage,
            .can_change_value = nullptr,
            .request_change_value = M_RequestChange,
        },
        .target = &g_Config.audio.sound_volume,
        .min_value = 0,
        .max_value = 100,
        .delta_slow = 10,
        .delta_fast = 10,
    },
#if !KEEP_IT_SIMPLE
    {
        .target = &g_Config.audio.music_load_condition,
        .label_id = GS_ID(SOUND_SETTINGS_MUSIC_LOAD_CONDITION),
        .option_type = COT_ENUM,
        .misc = &m_MusicLoadConditionEnumEntries,
    },
    #if TR_VERSION == 1
    {
        .target = &g_Config.audio.enable_music_in_menu,
        .label_id = GS_ID(SOUND_SETTINGS_ENABLE_MUSIC_IN_MENU),
        .option_type = COT_BOOL,
    },
    {
        .target = &g_Config.audio.enable_pitched_sounds,
        .label_id = GS_ID(SOUND_SETTINGS_ENABLE_PITCHED_SOUNDS),
        .option_type = COT_BOOL,
    },
    {
        .target = &g_Config.audio.enable_music_in_inventory,
        .label_id = GS_ID(SOUND_SETTINGS_PAUSE_MUSIC_IN_INVENTORY),
        .option_type = COT_INVERTED_BOOL,
    },
    {
        .target = &g_Config.audio.fix_secrets_killing_music,
        .label_id = GS_ID(SOUND_SETTINGS_FIX_SECRETS_KILLING_MUSIC),
        .option_type = COT_BOOL,
    },
    {
        .target = &g_Config.audio.fix_speeches_killing_music,
        .label_id = GS_ID(SOUND_SETTINGS_FIX_SPEECHES_KILLING_MUSIC),
        .option_type = COT_BOOL,
    },
    {
        .target = &g_Config.audio.enable_ps_uzi_sfx,
        .label_id = GS_ID(SOUND_SETTINGS_PS_UZI_SFX),
        .option_type = COT_BOOL,
    },
    #endif
    {
        .target = &g_Config.audio.inventory_ambient_volume,
        .label_id = GS_ID(SOUND_SETTINGS_INVENTORY_AMBIENT_VOLUME),
        .option_type = COT_FLOAT,
        .min_value = 0,
        .max_value = 100,
        .delta_slow = 10,
        .delta_fast = 10,
        .custom_handler = {
            .format_value = M_FormatPercentage,
            .can_change_value = nullptr,
            .request_change_value = nullptr,
        },
    },
    {
        .target = &g_Config.audio.inventory_music_volume,
        .label_id = GS_ID(SOUND_SETTINGS_INVENTORY_MUSIC_VOLUME),
        .option_type = COT_FLOAT,
        .min_value = 0,
        .max_value = 100,
        .delta_slow = 10,
        .delta_fast = 10,
        .custom_handler = {
            .format_value = M_FormatPercentage,
            .can_change_value = nullptr,
            .request_change_value = nullptr,
        },
    },
    {
        .target = &g_Config.audio.underwater_ambient_volume,
        .label_id = GS_ID(SOUND_SETTINGS_UNDERWATER_AMBIENT_VOLUME),
        .option_type = COT_FLOAT,
        .min_value = 0,
        .max_value = 100,
        .delta_slow = 10,
        .delta_fast = 10,
        .custom_handler = {
            .format_value = M_FormatPercentage,
            .can_change_value = nullptr,
            .request_change_value = nullptr,
        },
    },
    {
        .target = &g_Config.audio.underwater_music_volume,
        .label_id = GS_ID(SOUND_SETTINGS_UNDERWATER_MUSIC_VOLUME),
        .option_type = COT_FLOAT,
        .min_value = 0,
        .max_value = 100,
        .delta_slow = 10,
        .delta_fast = 10,
        .custom_handler = {
            .format_value = M_FormatPercentage,
            .can_change_value = nullptr,
            .request_change_value = nullptr,
        },
    },
#endif
    {
        .target = nullptr,
    },
};
#undef KEEP_IT_SIMPLE

static const char *M_FormatPercentage(const UI_SETTINGS_OPTION *const option)
{
    const float value = *(float *)option->target;
    String_FormatInto(
        &m_TempString, &m_TempStringCap, "%.00f%%", value * 100.0f);
    return m_TempString;
}

static bool M_RequestChange(
    const UI_SETTINGS_OPTION *const option, const int32_t dir)
{
    float *value = (float *)option->target;
    (*value) += dir / 10.0f;
    if (option->target == &g_Config.audio.music_volume) {
        Music_SetVolume(*value);
    } else if (option->target == &g_Config.audio.sound_volume) {
        Sound_SetMasterVolume(*value);
    }
    Sound_Effect(SFX_MENU_PASSPORT, nullptr, SPM_ALWAYS);
    return true;
}

UI_SOUND_SETTINGS_STATE *UI_SoundSettings_Init(void)
{
    UI_SOUND_SETTINGS_STATE *s = Memory_Alloc(sizeof(UI_SETTINGS_STATE));
    s->row_pad = 10.0f;
    UI_Settings_Init(s, GS_ID(SOUND_SETTINGS_TITLE), m_SoundOptions);
    return s;
}

void UI_SoundSettings_Free(UI_SOUND_SETTINGS_STATE *const s)
{
    UI_Settings_Free(s);
    Memory_Free(s);
}

bool UI_SoundSettings_Control(UI_SOUND_SETTINGS_STATE *const s)
{
    return UI_Settings_Control(s);
}

void UI_SoundSettings(UI_SOUND_SETTINGS_STATE *const s)
{
    UI_Settings(s);
}
