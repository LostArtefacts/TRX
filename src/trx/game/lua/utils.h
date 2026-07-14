#pragma once

#include <trx/core/log.h>
#include <trx/game/objects.h>

#include <lauxlib.h>
#include <lua.h>

// What an API module's chunk is named. A runtime script is named without it.
#define LUA_API_CHUNK_PREFIX "@trx/"

// The script frame that reached the API. The wrappers in between vary in
// number, so the caller is found by name rather than at a fixed depth.
bool LUA_GetCallerInfo(lua_State *L, lua_Debug *ar);

// Creates trxc.<name> and fills it with `fns`. Terminate with
// {nullptr, nullptr}.
void LUA_RegisterModule(lua_State *L, const char *name, const luaL_Reg *fns);

// Pushes trxc.<name>, for a module with more on it than functions.
void LUA_GetModule(lua_State *L, const char *name);

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

// An argument the engine indexes one of its own tables with, unchecked.
// Narrowed only once it fits, so a wider value cannot wrap into range.
int32_t LUA_CheckRange(lua_State *L, int arg, int32_t count, const char *what);

// An object id, checked against the object table. Object_Get asserts on one
// outside it.
OBJECT_ID LUA_CheckObjectID(lua_State *L, int arg);

// The position table a script writes: { x = , y = , z = }. `arg` is the
// argument number the table came in on, and stays that in the error - reading a
// field with luaL_checkinteger blames the stack slot the field landed at
// instead, which is not an argument number at all.
XYZ_32 LUA_CheckXYZ(lua_State *L, int arg);

// The same, for a table that sits inside an argument rather than being one (an
// options table's `pos`). `idx` is where it sits; `arg` is what the error
// names.
XYZ_32 LUA_CheckXYZAt(lua_State *L, int idx, int arg);

void LUA_PushXYZ(lua_State *L, XYZ_32 value);
