#pragma once

#include <lua.h>

// Turns a script that never returns - an accidental `while true` - into a
// script error instead of a frozen game. A debug hook compares the clock
// against a budget of continuous Lua execution; the heartbeat, called once
// per frame from the shell, marks that the engine has control. Depends on
// nothing but Lua and the clock, so a unit test can drive it directly.

// Wall-clock seconds a script may run without handing control back.
#define LUA_GUARD_BUDGET_SEC 5.0

void LUA_Guard_Install(lua_State *L, double budget_sec);
void LUA_Guard_Heartbeat(void);
