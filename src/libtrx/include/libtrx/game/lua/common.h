#pragma once

#include <lualib.h>

// Result of evaluating a Lua chunk.
typedef struct {
    int code; // LUA_OK, LUA_ERRSYNTAX, LUA_ERRRUN, etc.
    char *message; // Error text (nullptr if code == LUA_OK).
} LUA_RESULT;

void LUA_Init(void);
void LUA_Shutdown(void);

// Evaluate a Lua code string. Caller must free the result with Lua_FreeResult.
LUA_RESULT Lua_Eval(const char *code);

// Free the LUA eval result.
void Lua_FreeResult(LUA_RESULT *result);
