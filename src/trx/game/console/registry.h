#pragma once

#include <trx/core/completion.h>
#include <trx/core/vector.h>
#include <trx/game/console/types.h>

void Console_Registry_Add(CONSOLE_COMMAND cmd);

// Drops every command. The commands all come from a Lua state and live only as
// long as it does, so the state clears them on shutdown and its scripts
// register them again on the next run.
void Console_Registry_Clear(void);

const CONSOLE_COMMAND *Console_Registry_Get(const char *cmdline);

// Adds a suggestion to `out` for every command spelling that begins with
// word_prefix, deduped and sorted.
void Console_Registry_Suggest(const char *word_prefix, COMPLETION *out);

// Retrieve a vector containing pointers to all registered console commands.
// The returned vector must be freed via Vector_Free().
VECTOR *Console_Registry_GetAll(void);
