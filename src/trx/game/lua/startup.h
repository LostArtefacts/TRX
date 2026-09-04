#pragma once

// Runs every Lua script in the engine's scripts/ directory in name order.
// Reports script errors on the console and continues with the remaining files.
void LUA_RunStartupScripts(void);
