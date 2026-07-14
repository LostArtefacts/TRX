#pragma once

#include <lua.h>

// Pushes one of the registry's C entrypoints - "seal" or "to_json". They are
// not on trx.api, so there is no name to reach them by from a script; see
// lua/capi/api.c. False if it was never handed over, and nothing is pushed.
bool LUA_API_PushEntrypoint(lua_State *L, const char *name);
