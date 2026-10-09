#include <trx/game/locale/common.h>

#include <trx/config.h>
#include <trx/core/filesystem.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/core/subsystem.h>
#include <trx/game/game_flow/common.h>
#include <trx/game/game_strings/lang_match.h>
#include <trx/game/game_strings/manager.h>
#include <trx/game/replay/test_replay.h>

#include <ctype.h>
#include <stdint.h>
#include <string.h>

static EVENT_MANAGER *m_EventManager = nullptr;
static int32_t m_ConfigListener = -1;

static EVENT_MANAGER *M_GetEventManager(void)
{
    if (m_EventManager == nullptr) {
        m_EventManager = EventManager_Create();
    }
    return m_EventManager;
}

static const char *M_GetAudioCode(void)
{
    const char *const setting = g_Config.audio.media_language;
    if (setting == nullptr || setting[0] == '\0'
        || String_Equivalent(setting, LOCALE_AUDIO_AUTO)) {
        return g_Config.language;
    }
    if (String_Equivalent(setting, LOCALE_AUDIO_RETAIL)) {
        return nullptr;
    }
    return setting;
}

static void M_FreeCodes(VECTOR *const codes)
{
    if (codes == nullptr) {
        return;
    }
    for (int32_t i = 0; i < codes->count; i++) {
        Memory_Free(*(char **)Vector_Get(codes, i));
    }
    Vector_Free(codes);
}

static bool M_IsLangCode(const char *const name)
{
    if (!isalpha((unsigned char)name[0]) || !isalpha((unsigned char)name[1])) {
        return false;
    }
    if (name[2] == '\0') {
        return true;
    }
    if (name[2] != '-') {
        return false;
    }
    const size_t region_len = strlen(&name[3]);
    if (region_len < 2 || region_len > 4) {
        return false;
    }
    for (size_t i = 0; i < region_len; i++) {
        if (!isalnum((unsigned char)name[3 + i])) {
            return false;
        }
    }
    return true;
}

static void M_AddCode(VECTOR *const codes, const char *const code)
{
    for (int32_t i = 0; i < codes->count; i++) {
        if (String_Equivalent(*(char **)Vector_Get(codes, i), code)) {
            return;
        }
    }
    char *const lowered = String_ToLower(code);
    Vector_Add(codes, &lowered);
}

static void M_AddDubFolders(VECTOR *const codes, const GAME_DYNAMIC_PATH path)
{
    VECTOR *const dirs = GamePath_GetSearchDirs(path);
    for (int32_t i = 0; i < dirs->count; i++) {
        char *dir_path = *(char **)Vector_Get(dirs, i);
        FS_DIR *const dir = FS_OpenDirectory(dir_path);
        const char *name = nullptr;
        while (dir != nullptr && (name = FS_ReadDirectory(dir)) != nullptr) {
            if (!M_IsLangCode(name)) {
                continue;
            }
            char *sub_path = String_Format("%s/%s", dir_path, name);
            if (FS_DirExists(sub_path)) {
                M_AddCode(codes, name);
            }
            Memory_FreePointer(&sub_path);
        }
        if (dir != nullptr) {
            FS_CloseDirectory(dir);
        }
        Memory_FreePointer(&dir_path);
    }
    Vector_Free(dirs);
}

static void M_AddDubFMVNames(VECTOR *const codes)
{
    const int32_t fmv_count = GF_GetFMVCount();
    for (int32_t num = 1; num <= fmv_count; num++) {
        const GF_FMV *const fmv = GF_GetFMV(num);
        for (int32_t i = 0; i < fmv->localized_path.entry_count; i++) {
            const LOCALE_PATH_ENTRY *const entry =
                &fmv->localized_path.entries[i];
            if (entry->lang == nullptr) {
                continue;
            }
            for (int32_t j = 0; j < entry->name_count; j++) {
                if (GamePath_PeekResolve(
                        GAME_DYNAMIC_PATH_FMV_FILE, entry->names[j])
                    != nullptr) {
                    M_AddCode(codes, entry->lang);
                    break;
                }
            }
        }
    }
}

static VECTOR *M_GetAvailableDubs(void)
{
    VECTOR *const codes = Vector_Create(sizeof(char *));
    M_AddDubFolders(codes, GAME_DYNAMIC_PATH_FMV_FILE);
    M_AddDubFolders(codes, GAME_DYNAMIC_PATH_CDAUDIO_FILE);
    M_AddDubFolders(codes, GAME_DYNAMIC_PATH_MUSIC_DIR);
    M_AddDubFMVNames(codes);
    return codes;
}

static void M_FireChange(const LOCALE_ROLE role)
{
    const EVENT event = {
        .name = "change",
        .sender = nullptr,
        .data = (void *)(intptr_t)role,
    };
    EventManager_Fire(M_GetEventManager(), &event);
}

static void M_HandleConfigChange(const EVENT *const event, void *const data)
{
    const bool text_changed =
        Config_Change_HasMirror(event->data, &g_Config.language);
    const bool audio_changed =
        Config_Change_HasMirror(event->data, &g_Config.audio.media_language);
    if (text_changed) {
        M_FireChange(LOCALE_ROLE_TEXT);
        M_FireChange(LOCALE_ROLE_SUBTITLES);
    }
    if (text_changed || audio_changed) {
        M_FireChange(LOCALE_ROLE_AUDIO);
    }
}

static void M_Init(void)
{
    if (m_ConfigListener < 0) {
        m_ConfigListener =
            Config_SubscribeChanges(M_HandleConfigChange, nullptr);
    }
}

static void M_Shutdown(void)
{
    if (m_ConfigListener >= 0) {
        Config_UnsubscribeChanges(m_ConfigListener);
        m_ConfigListener = -1;
    }
    if (m_EventManager != nullptr) {
        EventManager_Free(m_EventManager);
        m_EventManager = nullptr;
    }
}

LOCALE_CHAIN Locale_GetChain(const LOCALE_ROLE role)
{
    return LocaleChain_FromCode(Locale_GetCode(role));
}

const char *Locale_GetCode(const LOCALE_ROLE role)
{
    switch (role) {
    case LOCALE_ROLE_TEXT:
    case LOCALE_ROLE_SUBTITLES:
        return g_Config.language;
    case LOCALE_ROLE_AUDIO:
        return M_GetAudioCode();
    default:
        return nullptr;
    }
}

VECTOR *Locale_GetAvailable(const LOCALE_ROLE role)
{
    switch (role) {
    case LOCALE_ROLE_TEXT:
    case LOCALE_ROLE_SUBTITLES: {
        VECTOR *const codes = GameStringManager_GetAvailableLanguages();
        return codes != nullptr ? codes : Vector_Create(sizeof(char *));
    }
    case LOCALE_ROLE_AUDIO:
        return M_GetAvailableDubs();
    default:
        return Vector_Create(sizeof(char *));
    }
}

const char *Locale_GetLanguageName(const char *const code)
{
    return GameStringManager_GetLanguageName(code);
}

char *Locale_Find(
    const LOCALE_ROLE role, const char *const path, const LOCALE_FIND_FUNC func,
    void *const user_data)
{
    const LOCALE_CHAIN chain = Locale_GetChain(role);
    return LocaleChain_Find(&chain, path, func, user_data);
}

char *Locale_Resolve(
    const LOCALE_ROLE role, const GAME_DYNAMIC_PATH path, const char *const rel)
{
    const LOCALE_CHAIN chain = Locale_GetChain(role);
    return LocaleChain_Resolve(&chain, path, rel);
}

char *Locale_ResolvePath(
    const LOCALE_ROLE role, const GAME_DYNAMIC_PATH kind,
    const LOCALE_PATH *const path)
{
    const LOCALE_CHAIN chain = Locale_GetChain(role);
    return LocalePath_Resolve(path, &chain, kind);
}

int32_t Locale_SubscribeChanges(
    const EVENT_LISTENER listener, void *const user_data)
{
    return EventManager_Subscribe(
        M_GetEventManager(), "change", nullptr, listener, user_data);
}

void Locale_UnsubscribeChanges(const int32_t listener_id)
{
    if (m_EventManager != nullptr) {
        EventManager_Unsubscribe(m_EventManager, listener_id);
    }
}

void Locale_ApplySystemLanguage(const VECTOR *const preferred)
{
    // A replay is left out: what it plays back has to read the same on every
    // machine.
    if (!Config_IsFirstRun() || TestReplay_IsOpened()) {
        return;
    }
    VECTOR *const available = GameStringManager_GetAvailableLanguages();
    if (available == nullptr) {
        return;
    }
    const char *const match =
        GameStringLang_MatchPreferred(available, preferred);
    if (match != nullptr) {
        LOG_INFO("selecting language '%s' from system preferences", match);
        CONFIG_SET(g_Config.language, match);
    }
    M_FreeCodes(available);
}

REGISTER_SUBSYSTEM(.init = M_Init, .shutdown = M_Shutdown)
