#pragma once

#include <trx/core/value.h>

#include <lua.h>

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
