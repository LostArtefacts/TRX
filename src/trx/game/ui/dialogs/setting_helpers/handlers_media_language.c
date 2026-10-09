#include <trx/config.h>
#include <trx/config/registry.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/game/game_strings/entries.h>
#include <trx/game/locale/common.h>
#include <trx/game/ui/dialogs/settings_handlers.h>

static VECTOR *M_GetChoices(void)
{
    VECTOR *const choices = Locale_GetAvailable(LOCALE_ROLE_AUDIO);
    Vector_Insert(choices, 0, &(char *) { Memory_DupStr(LOCALE_AUDIO_RETAIL) });
    Vector_Insert(choices, 0, &(char *) { Memory_DupStr(LOCALE_AUDIO_AUTO) });
    return choices;
}

static void M_FreeChoices(VECTOR *const choices)
{
    for (int32_t i = 0; i < choices->count; i++) {
        Memory_Free(*(char **)Vector_Get(choices, i));
    }
    Vector_Free(choices);
}

static int32_t M_FindIndex(
    const VECTOR *const choices, const CONFIG_OPTION *const option)
{
    for (int32_t i = 0; i < choices->count; i++) {
        if (String_Equivalent(
                *(char **)Vector_Get(choices, i), option->value.as_str)) {
            return i;
        }
    }
    return -1;
}

static const char *M_FormatValue(
    const CONFIG_OPTION *const option, void *const user_data)
{
    const char *const code = option->value.as_str;
    if (code == nullptr || code[0] == '\0'
        || String_Equivalent(code, LOCALE_AUDIO_AUTO)) {
        return GS("settings/audio.media_language/auto");
    }
    if (String_Equivalent(code, LOCALE_AUDIO_RETAIL)) {
        return GS("settings/audio.media_language/retail");
    }
    const char *const name = Locale_GetLanguageName(code);
    return name != nullptr ? name : code;
}

static bool M_CanChangeValue(
    const CONFIG_OPTION *const option, const int32_t dir, void *const user_data)
{
    VECTOR *const choices = M_GetChoices();
    const int32_t idx = M_FindIndex(choices, option);
    // A dub no longer installed steps back onto the list.
    const bool result =
        idx < 0 || (idx + dir >= 0 && idx + dir < choices->count);
    M_FreeChoices(choices);
    return result;
}

static bool M_RequestChangeValue(
    CONFIG_OPTION *const option, const int32_t dir, void *const user_data)
{
    VECTOR *const choices = M_GetChoices();
    const int32_t idx = M_FindIndex(choices, option);
    const int32_t new_idx = idx < 0 ? 0 : idx + dir;
    if (new_idx < 0 || new_idx >= choices->count) {
        M_FreeChoices(choices);
        return false;
    }
    if (!Config_Option_SetFromString(
            option, *(char **)Vector_Get(choices, new_idx), false)) {
        LOG_WARNING("Failed to set the dub");
    }
    M_FreeChoices(choices);
    return true;
}

REGISTER_UI_SETTING_HANDLER(
        .key = "audio.media_language", .format_value = M_FormatValue,
        .can_change_value = M_CanChangeValue,
        .request_change_value = M_RequestChangeValue)
