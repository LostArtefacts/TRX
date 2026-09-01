#pragma once

#include <trx/core/log.h>
#include <trx/core/value.h>
#include <trx/game/objects.h>

#include <lauxlib.h>
#include <lua.h>

// What an API module's chunk is named. A runtime script is named without it.
#define LUA_API_CHUNK_PREFIX "@trx/api/"

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

// A wide integer argument, range-tested against [lo, hi] before it is narrowed,
// so a value past int32_t's width cannot wrap into range and name something the
// script did not ask for. Returns false with nothing pushed when out of range;
// the caller decides what that means (nil, an error).
bool LUA_CheckBoundedInt(
    lua_State *L, int arg, lua_Integer lo, lua_Integer hi, int32_t *out);

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

// Push an item as the handle trx.items hands out, so that a module other than
// that one can give a script an item to work with.
void LUA_PushItem(lua_State *L, int16_t item_num);

// Takes the function that turns three channels into a color value, which is
// where trx.math declares what a color is. Called once, as the API loads;
// without it a color is pushed as a plain table of channels.
void LUA_SetColorConstructor(lua_State *L, int idx);

// Pushes a TRX_VALUE onto the Lua stack in its natural Lua shape: a boolean, an
// integer, a number, an {x,y,z} table, a color, or a string (a null string as
// nil).
void LUA_PushValue(lua_State *L, const TRX_VALUE *value);

// The same, for a value read off a member: `owner_idx` and `key_idx` say which
// member of what it came from. A value that carries its own identity - a color
// - is bound to that member, so writing part of it writes the whole value back
// through the member's setter. Anything else is pushed as a copy.
void LUA_PushMemberValue(
    lua_State *L, const TRX_VALUE *value, int owner_idx, int key_idx);

// Reads the argument at `idx` as a value of `type`, the inverse of
// LUA_PushValue. Raises a Lua error if the argument is the wrong shape. A nil
// string reads as a null value.
TRX_VALUE LUA_CheckValue(lua_State *L, int idx, TRX_VALUE_TYPE type);

// Pushes a zero-based engine index to a script as a one-based number, or nil
// when it holds `sentinel` (the value that means "none"). Scripts count rooms
// and items from 1.
void LUA_PushOptIndex(lua_State *L, int32_t value, int32_t sentinel);
