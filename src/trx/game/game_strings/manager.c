#include <trx/game/game_strings/manager.h>

#include <trx/config.h>
#include <trx/core/filesystem.h>
#include <trx/core/json.h>
#include <trx/core/memory.h>
#include <trx/core/result.h>
#include <trx/core/strings.h>
#include <trx/core/subsystem.h>
#include <trx/core/utils.h>
#include <trx/core/vector.h>
#include <trx/debug.h>
#include <trx/game/game_flow/common.h>
#include <trx/game/game_strings/lang_match.h>
#include <trx/game/game_strings/table.h>
#include <trx/game/replay/test_replay.h>
#include <trx/game/shell.h>
#include <trx/game/shell/platform.h>

#include <string.h>

typedef struct {
    char *path;
    bool load_levels;
} M_FILE_ENTRY;

typedef struct {
    char *lang;
    VECTOR *files;
    char *display_name;
    char *extends;
} M_LANG_ENTRY;

static VECTOR *m_SourceFiles = nullptr;
static VECTOR *m_LangEntries = nullptr;
static EVENT_MANAGER *m_EventManager = nullptr;

static void M_ClearFileEntries(VECTOR *const files)
{
    for (int32_t i = 0; i < files->count; i++) {
        const M_FILE_ENTRY *const file_entry = Vector_Get(files, i);
        Memory_Free(file_entry->path);
    }
    Vector_Free(files);
}

static void M_ClearLanguageEntries(void)
{
    if (m_LangEntries != nullptr) {
        for (int32_t i = 0; i < m_LangEntries->count; i++) {
            const M_LANG_ENTRY *const lang_entry = Vector_Get(m_LangEntries, i);
            Memory_Free(lang_entry->lang);
            Memory_Free(lang_entry->display_name);
            Memory_Free(lang_entry->extends);
            M_ClearFileEntries(lang_entry->files);
        }
        Vector_Free(m_LangEntries);
        m_LangEntries = nullptr;
    }
}

static void M_ClearManager(void)
{
    M_ClearLanguageEntries();
    if (m_SourceFiles != nullptr) {
        M_ClearFileEntries(m_SourceFiles);
        m_SourceFiles = nullptr;
    }
}

static M_LANG_ENTRY *M_FindLangEntry(const char *const lang)
{
    for (int32_t i = 0; i < m_LangEntries->count; i++) {
        M_LANG_ENTRY *const lang_entry = Vector_Get(m_LangEntries, i);
        if (String_Equivalent(lang_entry->lang, lang)) {
            return lang_entry;
        }
    }
    return nullptr;
}

// Create a new entry with the given file for the given language.
static void M_AddPathForLang(
    const char *const lang, char *const path, const bool load_levels)
{
    const M_LANG_ENTRY *lang_entry = M_FindLangEntry(lang);
    if (lang_entry == nullptr) {
        M_LANG_ENTRY new_ent = {
            .lang = Memory_DupStr(lang),
            .files = Vector_Create(sizeof(M_FILE_ENTRY)),
        };
        Vector_Add(m_LangEntries, &new_ent);
        lang_entry = M_FindLangEntry(lang);
    }
    const M_FILE_ENTRY file_entry = {
        .path = path,
        .load_levels = load_levels,
    };
    Vector_Add(lang_entry->files, &file_entry);
}

static void M_LoadLanguageNames(void)
{
    if (m_LangEntries == nullptr) {
        return;
    }
    for (int32_t i = 0; i < m_LangEntries->count; i++) {
        M_LANG_ENTRY *const lang_entry = Vector_Get(m_LangEntries, i);
        Memory_FreePointer(&lang_entry->display_name);
        Memory_FreePointer(&lang_entry->extends);
        if (lang_entry->files->count <= 0) {
            continue;
        }
        const M_FILE_ENTRY *const file_entry = Vector_Get(lang_entry->files, 0);
        char *data = nullptr;
        size_t size = 0;
        if (!SHOULD(FS_Load(file_entry->path, &data, &size))) {
            continue;
        }
        JSON_PARSE_RESULT pr = { 0 };
        JSON_VALUE *const root = JSON_ParseEx(
            data, size, JSON_PARSE_FLAGS_ALLOW_JSON5, nullptr, nullptr, &pr);
        if (root != nullptr) {
            JSON_OBJECT *const obj = JSON_ValueAsObject(root);
            const char *const name =
                JSON_ObjectGetString(obj, "language_name", JSON_INVALID_STRING);
            if (name != JSON_INVALID_STRING) {
                lang_entry->display_name = Memory_DupStr(name);
            }
            const char *const ext =
                JSON_ObjectGetString(obj, "extends", JSON_INVALID_STRING);
            if (ext != JSON_INVALID_STRING) {
                lang_entry->extends = Memory_DupStr(ext);
            }
            JSON_ValueFree(root);
        } else {
            LOG_WARNING(
                "failed to parse 'language_name' in %s: %s", file_entry->path,
                JSON_GetErrorDescription(pr.error));
        }
        Memory_Free(data);
    }
}

static void M_ReorderLanguages(void)
{
    if (m_LangEntries->count > 1) {
        VECTOR *ordered = Vector_Create(sizeof(M_LANG_ENTRY));
        const M_LANG_ENTRY *en_orig = M_FindLangEntry("en");
        if (en_orig != nullptr) {
            M_LANG_ENTRY en_entry = *en_orig;
            Vector_Add(ordered, &en_entry);
        }
        VECTOR *others = Vector_Create(sizeof(M_LANG_ENTRY));
        for (int32_t i = 0; i < m_LangEntries->count; ++i) {
            const M_LANG_ENTRY *entry = Vector_Get(m_LangEntries, i);
            if (en_orig != nullptr && String_Equivalent(entry->lang, "en")) {
                continue;
            }
            M_LANG_ENTRY e = *entry;
            Vector_Add(others, &e);
        }
        for (int32_t i = 0; i + 1 < others->count; ++i) {
            for (int32_t j = 0; j + 1 < others->count - i; ++j) {
                M_LANG_ENTRY *a = Vector_Get(others, j);
                M_LANG_ENTRY *b = Vector_Get(others, j + 1);
                const char *an = a->display_name ? a->display_name : "";
                const char *bn = b->display_name ? b->display_name : "";
                if (strcmp(an, bn) > 0) {
                    Vector_Swap(others, j, j + 1);
                }
            }
        }
        for (int32_t i = 0; i < others->count; ++i) {
            M_LANG_ENTRY *entry = Vector_Get(others, i);
            Vector_Add(ordered, entry);
        }
        Vector_Free(others);
        Vector_Free(m_LangEntries);
        m_LangEntries = ordered;
    }
}

// Recursive load of language chain (handles 'extends' fallback between
// dialects)
static RESULT M_ReloadLangRec(const char *const lang, VECTOR *const visited)
{
    for (int32_t i = 0; i < visited->count; i++) {
        const char *const prev = *(char **)Vector_Get(visited, i);
        FAIL_IF(
            String_Equivalent(prev, lang),
            "cyclic language extends detected: %s", lang);
    }
    Vector_Add(visited, &lang);
    M_LANG_ENTRY *const entry = M_FindLangEntry(lang);
    FAIL_IF(entry == nullptr, "unknown language: %s", lang);
    if (entry->extends) {
        MUST(
            M_ReloadLangRec(entry->extends, visited), "extended by '%s'", lang);
    }
    for (int32_t i = 0; i < entry->files->count; i++) {
        const M_FILE_ENTRY *const fe = Vector_Get(entry->files, i);
        MUST(GameStringTable_Load(fe->path, fe->load_levels));
    }
    return OK;
}

static void M_Init(void)
{
    m_EventManager = EventManager_Create();
    M_ClearManager();
    m_SourceFiles = Vector_Create(sizeof(M_FILE_ENTRY));
}

static void M_Shutdown(void)
{
    if (m_EventManager != nullptr) {
        EventManager_Free(m_EventManager);
        m_EventManager = nullptr;
    }
    GameStringTable_Shutdown();
    M_ClearManager();
}

// Clear all previously set source strings files.
static void M_ClearSourceFiles(void)
{
    M_ClearManager();
    m_SourceFiles = Vector_Create(sizeof(M_FILE_ENTRY));
}

// Add a source strings file for language discovery and loading.
// base_path: path to a base strings JSON5 file.
// load_levels: true to load level entries from this source; false otherwise.
static void M_AddSourceFile(const char *const base_path, const bool load_levels)
{
    ASSERT(m_SourceFiles != nullptr);
    if (base_path == nullptr) {
        return;
    }
    const M_FILE_ENTRY fe = {
        .path = Memory_DupStr(base_path),
        .load_levels = load_levels,
    };
    Vector_Add(m_SourceFiles, &fe);
}

static void M_DiscoverLanguages(void)
{
    if (m_SourceFiles == nullptr) {
        return;
    }
    M_ClearLanguageEntries();
    m_LangEntries = Vector_Create(sizeof(M_LANG_ENTRY));

    for (int32_t i = 0; i < m_SourceFiles->count; ++i) {
        const M_FILE_ENTRY *src = Vector_Get(m_SourceFiles, i);
        char *dir = FS_GetParentDirectory(src->path);
        const char *const base = FS_GetBaseName(src->path);
        const char *ext = strrchr(base, '.');
        if (dir == nullptr || ext == nullptr) {
            Memory_Free(dir);
            continue;
        }
        size_t stem_len = (size_t)(ext - base);
        size_t ext_len = strlen(ext);

        FS_DIR *dh = FS_OpenDirectory(dir);
        if (dh != nullptr) {
            const char *ent;
            while ((ent = FS_ReadDirectory(dh))) {
                if (ent[0] == '.') {
                    continue;
                }
                size_t name_len = strlen(ent);
                if (name_len < stem_len + ext_len) {
                    continue;
                }
                if (strncmp(ent, base, stem_len) != 0
                    || strcmp(ent + name_len - ext_len, ext) != 0) {
                    continue;
                }
                char *code;
                if (name_len == stem_len + ext_len) {
                    code = Memory_DupStr("en");
                } else if (ent[stem_len] == '-') {
                    size_t code_len = name_len - stem_len - ext_len - 1;
                    code = String_Format(
                        "%.*s", (int32_t)code_len, ent + stem_len + 1);
                } else {
                    continue;
                }
                char *path = String_Format("%s/%s", dir, ent);
                M_AddPathForLang(code, path, src->load_levels);
                Memory_Free(code);
            }
            FS_CloseDirectory(dh);
        }
        Memory_Free(dir);
    }

    M_LoadLanguageNames();
    M_ReorderLanguages();
}

static void M_FreeCodes(VECTOR *const codes)
{
    for (int32_t i = 0; i < codes->count; i++) {
        Memory_Free(*(char **)Vector_Get(codes, i));
    }
    Vector_Free(codes);
}

// A player who has never launched the game has never said what language they
// want, so the one their system is set to stands in for the answer. A replay
// is left out of it: what it plays back has to read the same on every machine.
static void M_ApplySystemLanguage(void)
{
    if (!Config_IsFirstRun() || TestReplay_IsOpened()) {
        return;
    }
    VECTOR *const available = GameStringManager_GetAvailableLanguages();
    if (available == nullptr) {
        return;
    }
    VECTOR *const preferred = Shell_GetPreferredLanguages();
    const char *const match =
        GameStringLang_MatchPreferred(available, preferred);
    if (match != nullptr) {
        LOG_INFO("selecting language '%s' from system preferences", match);
        CONFIG_SET(g_Config.language, match);
    }
    M_FreeCodes(preferred);
    M_FreeCodes(available);
}

RESULT GameStringManager_LoadForMod(const SHELL_MOD *const mod)
{
    M_ClearSourceFiles();

    const char *const common_strings_path = Shell_GetCommonStringsPath();
    FAIL_IF(common_strings_path == nullptr, "Missing common strings file");
    M_AddSourceFile(common_strings_path, false);

    if (mod->base_mod != nullptr) {
        char *base_strings_path = Shell_GetBaseGameStringsPath(mod);
        FAIL_IF(
            base_strings_path == nullptr,
            "Missing base mod strings file for '%s'", mod->name);
        M_AddSourceFile(base_strings_path, false);
        Memory_FreePointer(&base_strings_path);
    }

    char *mod_strings_path = Shell_GetGameStringsPath(mod);
    FAIL_IF(
        mod_strings_path == nullptr,
        "Missing strings file for selected mod '%s'", mod->name);
    M_AddSourceFile(mod_strings_path, true);
    Memory_FreePointer(&mod_strings_path);

    M_DiscoverLanguages();
    M_ApplySystemLanguage();
    return GameStringManager_ReloadLanguage(g_Config.language);
}

VECTOR *GameStringManager_GetAvailableLanguages(void)
{
    if (m_LangEntries == nullptr) {
        return nullptr;
    }
    VECTOR *const out = Vector_Create(sizeof(char *));
    for (int32_t i = 0; i < m_LangEntries->count; i++) {
        const M_LANG_ENTRY *const lang_entry = Vector_Get(m_LangEntries, i);
        char *const c = Memory_DupStr(lang_entry->lang);
        Vector_Add(out, &c);
    }
    return out;
}

RESULT GameStringManager_ReloadLanguage(const char *lang)
{
    const M_LANG_ENTRY *const base_entry =
        m_LangEntries ? M_FindLangEntry(lang) : nullptr;
    if (base_entry == nullptr) {
        LOG_WARNING("language '%s' not found, defaulting to base", lang);
        lang = "en";
    }
    GameStringTable_Shutdown();
    GameStringTable_Init();
    VECTOR *const visited = Vector_Create(sizeof(char *));
    const RESULT result = M_ReloadLangRec(lang, visited);
    Vector_Free(visited);
    if (IS_OK(result)) {
        GameStringTable_Apply(GF_GetCurrentLevel());
        if (m_EventManager != nullptr) {
            const EVENT event = {
                .name = "reload_language",
                .sender = nullptr,
                .data = (void *)lang,
            };
            EventManager_Fire(m_EventManager, &event);
        }
    }
    return result;
}

const char *GameStringManager_GetLanguageName(const char *const code)
{
    if (m_LangEntries == nullptr || code == nullptr) {
        return nullptr;
    }
    const M_LANG_ENTRY *const ent = M_FindLangEntry(code);
    return ent != nullptr ? ent->display_name : nullptr;
}

int32_t GameStringManager_SubscribeReload(
    const EVENT_LISTENER listener, void *const user_data)
{
    ASSERT(m_EventManager != nullptr);
    return EventManager_Subscribe(
        m_EventManager, "reload_language", nullptr, listener, user_data);
}

void GameStringManager_UnsubscribeReload(const int32_t listener_id)
{
    if (m_EventManager != nullptr) {
        EventManager_Unsubscribe(m_EventManager, listener_id);
    }
}

REGISTER_BASE_SUBSYSTEM(.init = M_Init, .shutdown = M_Shutdown)
