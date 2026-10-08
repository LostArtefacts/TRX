#pragma once

#include <trx/core/log.h>

#include <lua.h>

// What an API module's chunk is named. A runtime script is named without it.
#define LUA_API_CHUNK_PREFIX "@trx/api/"

// The script frame that reached the API. The wrappers in between vary in
// number, so the caller is found by name rather than at a fixed depth.
bool LUA_GetCallerInfo(lua_State *L, lua_Debug *ar);

// What a log bridge takes: a level, a message, and where the call came from.
// `ar` is what `src` points into, so the whole struct has to outlive the log
// call.
typedef struct {
    LOG_LEVEL level;
    const char *msg;
    lua_Debug ar;
    const char *src;
    const char *func;
    int line;
} LUA_LOG_CALL;

// Reads (level, msg) off the stack and resolves the caller, falling back to "?"
// for a frame that cannot be named.
void LUA_CheckLogCall(lua_State *L, LUA_LOG_CALL *out);
