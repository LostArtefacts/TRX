// The game string table, reduced to a handful of entries: one plain, one with a
// placeholder, one with two, and one a translator wrote a bare percent sign
// into. The real one is loaded from cfg/base_strings.json5 and its
// translations; what is under test is what the surface does with these and with
// a key it does not have.

#include <trx/game/game_strings/entries.h>
#include <trx/game/game_strings/manager.h>

#include <string.h>

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
    // The help command composes its output from these, so its tests need them
    // resolved rather than left nil.
    if (strcmp(key, "console/argparse/aliases") == 0) {
        return "Aliases: %s";
    }
    if (strcmp(key, "console/cmd/help/list") == 0) {
        return "Available commands:";
    }
    if (strcmp(key, "console/cmd/help/unknown_command") == 0) {
        return "Unknown command: %s";
    }
    return nullptr;
}
