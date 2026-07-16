// The game string table, reduced to a handful of entries: one plain, one with a
// placeholder, one with two, and one a translator wrote a bare percent sign
// into. The real one is loaded from cfg/base_strings.json5 and its
// translations; what is under test is what the surface does with these and with
// a key it does not have.

#include <trx/config/types.h>
#include <trx/game/game_strings/entries.h>
#include <trx/game/game_strings/manager.h>

#include <string.h>

CONFIG g_Config;

// trx.locale.reload() reloads the language files from disk. There is no disk
// here, so report success without doing anything.
bool GameStringManager_ReloadLanguage(const char *const lang)
{
    return true;
}

const char *GameString_Get(const char *const key)
{
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
