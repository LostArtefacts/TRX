#pragma once

#include <trx/core/result.h>

// Runs loose scripts in name order, then each subdirectory's init.lua. The
// first script that fails stops the rest and reports failure.
RESULT LUA_RunStartupScripts(void);

// Return the directory of the running script, or null for a loose script. The
// string remains valid until the script returns.
const char *LUA_GetStartupScriptDir(void);
