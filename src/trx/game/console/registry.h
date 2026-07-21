#pragma once

#include <trx/core/completion.h>
#include <trx/core/utils.h>
#include <trx/core/vector.h>
#include <trx/game/console/types.h>

#include <stddef.h>

void Console_Registry_Add(CONSOLE_COMMAND cmd);

// Remove every command that dispatches through the given proc. A command
// declared in C lives for the whole process; one a Lua state registers lives
// only as long as that state, and is dropped when the state shuts down, so its
// script can register it again on the next run.
void Console_Registry_RemoveByProc(
    COMMAND_RESULT (*proc)(const COMMAND_CONTEXT *ctx));

const CONSOLE_COMMAND *Console_Registry_Get(const char *cmdline);

// Adds a suggestion to `out` for every command spelling that begins with
// word_prefix, deduped and sorted.
void Console_Registry_Suggest(const char *word_prefix, COMPLETION *out);

// Retrieve a vector containing pointers to all registered console commands.
// The returned vector must be freed via Vector_Free().
VECTOR *Console_Registry_GetAll(void);

#define REGISTER_CONSOLE_COMMAND(prefix_, proc_, help_)                        \
    __attribute__((__constructor__)) static void CONCAT(                       \
        M_Register_, __LINE__)(void)                                           \
    {                                                                          \
        Console_Registry_Add((CONSOLE_COMMAND) {                               \
            .prefix = prefix_, .proc = proc_, .help_id = help_ });             \
    }
