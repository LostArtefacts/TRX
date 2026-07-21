#pragma once

#include <trx/core/completion.h>

// Picks the completer that applies at `caret` in `line`: the command word
// completes against registered names, a token past it against the arguments the
// typed command declares. The prompt calls the returned completer and applies
// the chosen suggestion through Completion_Apply.
COMPLETER Console_GetCompleter(const char *line, int32_t caret);
