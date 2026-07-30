#pragma once

#include <lualib.h>

// The state LUA_Eval and LUA_EvalFile run against, which is the test's own.
void FakeLua_SetState(lua_State *L);
