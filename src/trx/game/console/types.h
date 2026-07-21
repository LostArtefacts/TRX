#pragma once
#include <trx/core/completion.h>
#include <trx/game/console/enum.h>
#include <trx/game/game_strings/entries.h>

typedef struct {
    const struct CONSOLE_COMMAND *cmd;
    const char *prefix;
    const char *args;
} COMMAND_CONTEXT;

typedef struct CONSOLE_COMMAND {
    const char *prefix;
    COMMAND_RESULT (*proc)(const COMMAND_CONTEXT *ctx);
    // Fills `out` for the argument under the caret, in the region past the
    // command word, with a region-relative span - or nullptr when the command
    // takes none. Mirrors proc: a Lua command points this at the bridge that
    // forwards to the script's completer, as proc forwards to its handler.
    void (*complete)(
        const struct CONSOLE_COMMAND *cmd, const char *text, int32_t caret,
        COMPLETION *out);
    GAME_STRING_ID help_id;
    // Other spellings that reach the same command, comma-joined for display, or
    // nullptr. Only the main spelling carries this and appears in the listing;
    // the aliases dispatch but stay out of it.
    const char *aliases;
} CONSOLE_COMMAND;
