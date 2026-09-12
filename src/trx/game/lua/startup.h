#pragma once

// Run loose scripts in name order, then each subdirectory's init.lua. Report
// errors on the console and continue with the remaining scripts.
void LUA_RunStartupScripts(void);

// Return the directory of the running script, or null for a loose script. The
// string remains valid until the script returns.
const char *LUA_GetStartupScriptDir(void);
