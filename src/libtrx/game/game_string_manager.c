#include "game/game_string_manager.h"

#include "debug.h"
#include "filesystem.h"
#include "game/game_string_table.h"
#include "json.h"
#include "memory.h"
#include "strings.h"
#include "utils.h"
#include "vector.h"

#include <string.h>

typedef struct {
    char *path;
    bool load_levels;
} M_FILE_ENTRY;

typedef struct {
    char *lang;
    VECTOR *files;
    char *display_name;
} M_LANG_ENTRY;

static char *m_OGBase = nullptr;
static char *m_ModBase = nullptr;
static VECTOR *m_LangEntries = nullptr;

static void M_ClearFileEntries(VECTOR *files);
static void M_ClearManager(void);
static M_LANG_ENTRY *M_FindLangEntry(const char *lang);
static void M_AddPathForLang(const char *lang, char *path, bool load_levels);
static void M_ScanBase(const char *base_path, bool load_levels_for_base);
static void M_LoadLanguageNames(void);

static void M_ClearFileEntries(VECTOR *const files)
{
    for (int32_t i = 0; i < files->count; i++) {
        const M_FILE_ENTRY *const file_entry = Vector_Get(files, i);
        Memory_Free(file_entry->path);
    }
    Vector_Free(files);
}

static void M_ClearManager(void)
{
    if (m_LangEntries != nullptr) {
        for (int32_t i = 0; i < m_LangEntries->count; i++) {
            const M_LANG_ENTRY *const lang_entry = Vector_Get(m_LangEntries, i);
            Memory_Free(lang_entry->lang);
            Memory_Free(lang_entry->display_name);
            M_ClearFileEntries(lang_entry->files);
        }
        Vector_Free(m_LangEntries);
        m_LangEntries = nullptr;
    }
    Memory_FreePointer(&m_OGBase);
    Memory_FreePointer(&m_ModBase);
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

// Search for language variants inside the parent directory of a given file.
static void M_ScanBase(
    const char *const base_path, const bool load_levels_for_base)
{
    void *dir_handle = nullptr;
    char *const dir = File_GetParentDirectory(base_path);
    if (dir == nullptr) {
        LOG_WARNING("cannot get directory for '%s'", base_path);
        return;
    }

    const char *base_name =
        MAX(strrchr(base_path, '\\'), strrchr(base_path, '/'));
    base_name = base_name != nullptr ? base_name + 1 : base_path;
    const char *const ext = strrchr(base_name, '.');
    if (ext == nullptr) {
        goto cleanup;
    }
    const size_t base_len = (size_t)(ext - base_name);

    dir_handle = File_OpenDirectory(dir);
    const char *entry;
    while ((entry = File_ReadDirectory(dir_handle))) {
        if (entry[0] == '.') {
            continue;
        }
        if (!String_EndsWith(entry, ext)
            || strncmp(entry, base_name, base_len) != 0) {
            continue;
        }

        // Got a match - add the entry to the relevant language table
        const size_t name_len = strlen(entry);
        if (name_len == base_len + strlen(ext)) {
            char *const lang_path = String_Format("%s/%s", dir, entry);
            M_AddPathForLang("en", lang_path, load_levels_for_base);
        } else if (entry[base_len] == '-') {
            const size_t code_len = name_len - base_len - strlen(ext) - 1;
            char *const code =
                String_Format("%.*s", (int32_t)code_len, entry + base_len + 1);
            char *const lang_path = String_Format("%s/%s", dir, entry);
            M_AddPathForLang(code, lang_path, load_levels_for_base);
            Memory_Free(code);
        }
    }

cleanup:
    if (dir_handle != nullptr) {
        File_CloseDirectory(dir_handle);
    }
    Memory_Free(dir);
}

// Load the 'language_name' field from each language's JSON5 strings file.
static void M_LoadLanguageNames(void)
{
    if (m_LangEntries == nullptr) {
        return;
    }
    for (int32_t i = 0; i < m_LangEntries->count; i++) {
        M_LANG_ENTRY *const lang_entry = Vector_Get(m_LangEntries, i);
        Memory_FreePointer(&lang_entry->display_name);
        if (lang_entry->files->count <= 0) {
            continue;
        }
        const M_FILE_ENTRY *const file_entry = Vector_Get(lang_entry->files, 0);
        char *data = nullptr;
        size_t size = 0;
        if (!File_Load(file_entry->path, &data, &size) || data == nullptr) {
            continue;
        }
        JSON_PARSE_RESULT pr = { 0 };
        JSON_VALUE *const root = JSON_ParseEx(
            data, size, JSON_PARSE_FLAGS_ALLOW_JSON5, nullptr, nullptr, &pr);
        if (root != nullptr) {
            JSON_OBJECT *obj = JSON_ValueAsObject(root);
            const char *name =
                JSON_ObjectGetString(obj, "language_name", JSON_INVALID_STRING);
            if (name != JSON_INVALID_STRING) {
                lang_entry->display_name = Memory_DupStr(name);
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

void GameStringManager_Init(void)
{
    M_ClearManager();
    m_LangEntries = Vector_Create(sizeof(M_LANG_ENTRY));
}

void GameStringManager_Shutdown(void)
{
    GameStringTable_Shutdown();
    M_ClearManager();
}

void GameStringManager_SetBaseFiles(
    const char *const og_base_path, const char *const mod_base_path)
{
    if (m_LangEntries == nullptr) {
        GameStringManager_Init();
    }
    M_ClearManager();
    m_LangEntries = Vector_Create(sizeof(M_LANG_ENTRY));
    m_OGBase = Memory_DupStr(og_base_path);
    m_ModBase = Memory_DupStr(mod_base_path);
    bool same = String_Equivalent(m_OGBase, m_ModBase);
    if (same) {
        M_ScanBase(m_ModBase, true);
    } else {
        M_ScanBase(m_OGBase, false);
        M_ScanBase(m_ModBase, true);
    }
    M_LoadLanguageNames();

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

void GameStringManager_ReloadLanguage(const char *const lang)
{
    const M_LANG_ENTRY *lang_entry =
        m_LangEntries != nullptr ? M_FindLangEntry(lang) : nullptr;
    GameStringTable_Shutdown();
    GameStringTable_Init();
    if (lang_entry == nullptr) {
        LOG_WARNING("language '%s' not found, defaulting to base", lang);
        lang_entry = M_FindLangEntry("en");
    }
    if (lang_entry != nullptr) {
        for (int32_t i = 0; i < lang_entry->files->count; i++) {
            const M_FILE_ENTRY *const file_entry =
                Vector_Get(lang_entry->files, i);
            GameStringTable_Load(file_entry->path, file_entry->load_levels);
        }
        GameStringTable_Apply(nullptr);
    }
}

const char *GameStringManager_GetLanguageName(const char *const code)
{
    if (m_LangEntries == nullptr || code == nullptr) {
        return nullptr;
    }
    const M_LANG_ENTRY *const ent = M_FindLangEntry(code);
    return ent != nullptr ? ent->display_name : nullptr;
}
