#pragma once

#include <lauxlib.h>
#include <lua.h>

// Creates trxc.<name> and fills it with `fns`. Terminate with
// {nullptr, nullptr}.
void LUA_RegisterModule(lua_State *L, const char *name, const luaL_Reg *fns);

// Pushes trxc.<name>, for a module with more on it than functions.
void LUA_GetModule(lua_State *L, const char *name);
