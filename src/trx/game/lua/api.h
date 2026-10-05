#pragma once

#include <lua.h>

// Pushes one of the C entrypoints api.lua hands over, such as "seal". They are
// not on trx.api, so there is no name to reach them by from a script; see
// lua/capi/api.c. False if it was never handed over, and nothing is pushed.
bool LUA_API_PushEntrypoint(lua_State *L, const char *name);
