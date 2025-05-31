#include "game/ui/dialogs/sound_settings.h"

#include "config.h"
#include "game/game_string.h"
#include "game/music.h"
#include "game/sound.h"
#include "memory.h"
#include "strings.h"
#include "utils.h"

// Custom handlers for changing sound settings.
static bool M_CanChange(const UI_SETTINGS_OPTION *option, int32_t dir);
static bool M_RequestChange(const UI_SETTINGS_OPTION *option, int32_t dir);

static const UI_SETTINGS_OPTION m_SoundOptions[] = {
    {
        .option_type = COT_INT32,
        .label_id = GS_ID(SOUND_DIALOG_SOUND),
        .custom_handler = {
            .format_value = nullptr,
            .can_change_value = M_CanChange,
            .request_change_value = M_RequestChange,
        },
        .target = &g_Config.audio.music_volume,
        .min_value = SOUND_MIN_VOLUME,
        .max_value = SOUND_MAX_VOLUME,
    },
    {
        .option_type = COT_INT32,
        .label_id = GS_ID(SOUND_DIALOG_MUSIC),
        .custom_handler = {
            .format_value = nullptr,
            .can_change_value = M_CanChange,
            .request_change_value = M_RequestChange,
        },
        .target = &g_Config.audio.sound_volume,
        .min_value = MUSIC_MIN_VOLUME,
        .max_value = MUSIC_MAX_VOLUME,
    },
    {
        .target = NULL,
    },
};

UI_SOUND_SETTINGS_STATE *UI_SoundSettings_Init(void)
{
    UI_SOUND_SETTINGS_STATE *s = Memory_Alloc(sizeof(UI_SETTINGS_STATE));
    s->row_pad = 10.0f;
    UI_Settings_Init(s, GS_ID(SOUND_DIALOG_TITLE), m_SoundOptions);
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

static bool M_CanChange(
    const UI_SETTINGS_OPTION *const option, const int32_t dir)
{
    const int32_t *value = option->target;
    if (option->label_id == GS_ID(SOUND_DIALOG_SOUND)) {
        if (dir < 0) {
            return *value > MUSIC_MIN_VOLUME;
        } else {
            return *value < MUSIC_MAX_VOLUME;
        }
    } else {
        if (dir < 0) {
            return *value > SOUND_MIN_VOLUME;
        } else {
            return *value < SOUND_MAX_VOLUME;
        }
    }
}

static bool M_RequestChange(
    const UI_SETTINGS_OPTION *const option, const int32_t dir)
{
    int32_t *value = option->target;
    if (!M_CanChange(option, dir)) {
        return false;
    }
    *value += dir;
    if (option->label_id == GS_ID(SOUND_DIALOG_SOUND)) {
        Music_SetVolume(*value);
    } else {
        Sound_SetMasterVolume(*value);
    }
    Sound_Effect(SFX_MENU_PASSPORT, NULL, SPM_ALWAYS);
    return true;
}
