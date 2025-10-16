#pragma once

#include <lauxlib.h>

// Provide luaL_getsubtable for Lua versions before 5.2 (LuaJIT compatibility)
int luaL_getsubtable(lua_State *L, int idx, const char *fname);
