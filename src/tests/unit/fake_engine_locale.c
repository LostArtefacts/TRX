// The game string table, reduced to what a surface test needs: the fixtures
// below - one plain, one with a placeholder, one with two, and one a translator
// wrote a bare percent sign into - plus whatever the scripts under test declare
// through trx.locale.declare(). The real one starts from
// game_strings/entries.def and has cfg/base_strings.json5 and its translations
// layered over it.

#include <trx/game/game_strings/entries.h>
#include <trx/game/game_strings/manager.h>

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *key;
    char *value;
} M_ENTRY;

// Enough for every module and command one test loads.
static M_ENTRY m_Entries[256];
static int32_t m_EntryCount = 0;

// trx.locale.reload() reloads the language files from disk. There is no disk
// here, so report success without doing anything.
bool GameStringManager_ReloadLanguage(const char *const lang)
{
    return true;
}

void GameString_Define(const char *const key, const char *const value)
{
    for (int32_t i = 0; i < m_EntryCount; i++) {
        if (strcmp(m_Entries[i].key, key) == 0) {
            free(m_Entries[i].value);
            m_Entries[i].value = strdup(value);
            return;
        }
    }
    assert(m_EntryCount < (int32_t)(sizeof(m_Entries) / sizeof(m_Entries[0])));
    m_Entries[m_EntryCount].key = strdup(key);
    m_Entries[m_EntryCount].value = strdup(value);
    m_EntryCount++;
}

const char *GameString_Get(const char *const key)
{
    for (int32_t i = 0; i < m_EntryCount; i++) {
        if (strcmp(m_Entries[i].key, key) == 0) {
            return m_Entries[i].value;
        }
    }
    if (strcmp(key, "test/plain") == 0) {
        return "Plain text";
    }
    if (strcmp(key, "test/formatted") == 0) {
        return "Text with %d in it";
    }
    if (strcmp(key, "test/two") == 0) {
        return "%d of %d";
    }
    if (strcmp(key, "test/percent") == 0) {
        return "100% of the text";
    }
    return nullptr;
}
