#include <trx/config.h>
#include <trx/config/registry.h>
#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/game/game_strings/manager.h>
#include <trx/game/ui/dialogs/settings_handlers.h>

#include <stdlib.h>

static VECTOR *m_Languages = nullptr;

static void M_Language_Cleanup(void)
{
    // Free the languages vector and its strings.
    if (m_Languages != nullptr) {
        for (int32_t i = 0; i < m_Languages->count; i++) {
            char *lang = *(char **)Vector_Get(m_Languages, i);
            Memory_Free(lang);
        }
        Vector_Free(m_Languages);
        m_Languages = nullptr;
    }
}

static const VECTOR *M_Language_GetLanguages(void)
{
    if (m_Languages == nullptr) {
        // Initialize available languages for the language option.
        m_Languages = GameStringManager_GetAvailableLanguages();
        atexit(M_Language_Cleanup);
    }
    return m_Languages;
}

static int32_t M_Language_FindIndex(const CONFIG_OPTION *const option)
{
    const VECTOR *const langs = M_Language_GetLanguages();
    const char *const cur = option->value.as_str;
    for (int32_t i = 0; i < langs->count; i++) {
        const char *const lang = *(char **)Vector_Get(langs, i);
        if (String_Equivalent(lang, cur)) {
            return i;
        }
    }
    return -1;
}

static const char *M_Language_FormatValue(
    const CONFIG_OPTION *const option, void *const user_data)
{
    const char *const code = option->value.as_str;
    const char *const name = GameStringManager_GetLanguageName(code);
    return name != nullptr ? name : code;
}

static bool M_Language_CanChangeValue(
    const CONFIG_OPTION *const option, const int32_t dir, void *const user_data)
{
    const VECTOR *const langs = M_Language_GetLanguages();
    const int32_t idx = M_Language_FindIndex(option);
    if (idx < 0) {
        // If the language from the user config is no longer on the list (the
        // file was deleted), let the player return to the default language
        return true;
    }
    if (langs->count < 2) {
        return false;
    }
    return idx + dir >= 0 && idx + dir < langs->count;
}

static bool M_Language_RequestChangeValue(
    CONFIG_OPTION *const option, const int32_t dir, void *const user_data)
{
    const VECTOR *const langs = M_Language_GetLanguages();
    if (!M_Language_CanChangeValue(option, dir, user_data)) {
        return false;
    }
    const char *new_lang;
    const int32_t idx = M_Language_FindIndex(option);
    if (idx != -1) {
        new_lang = *(char **)Vector_Get(langs, idx + dir);
    } else {
        // If the language from the user config is no longer on the list (the
        // file was deleted), default to the first entry, which is English
        new_lang = *(char **)Vector_Get(langs, 0);
    }
    Config_Option_SetFromString(option, new_lang, false);
    GameStringManager_ReloadLanguage(new_lang);
    return true;
}

REGISTER_UI_SETTING_HANDLER(
        .key = "language", .format_value = M_Language_FormatValue,
        .can_change_value = M_Language_CanChangeValue,
        .request_change_value = M_Language_RequestChangeValue)
