#pragma once

#include <trx/core/math/types.h>
#include <trx/game/objects/ids.h>

#include <lua.h>

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

// Pushes a zero-based engine index to a script as a one-based number, or nil
// when it holds `sentinel` (the value that means "none"). Scripts count rooms
// and items from 1.
void LUA_PushOptIndex(lua_State *L, int32_t value, int32_t sentinel);

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
