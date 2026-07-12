#pragma once

#include <trx/core/field.h>

#include <lauxlib.h>
#include <lualib.h>

// Generic Lua bridge over FIELD_DESC.
//
// Registering a type creates its metatable with EMPTY public tables: no field,
// method or computed member is reachable until a script declares it (see
// trx.api.type in data/scripting/api.lua). The declaration is what populates
// the metatable, so the public API is coined in Lua, while dispatch stays in C
// - routing field reads through Lua costs ~1.5x on the hottest path scripts
// have.

typedef struct LUA_STRUCT_REF {
    const TYPE_DESC *type;
    // Resolves the handle to a live pointer, or nullptr if it went stale (the
    // slot was recycled, the level was unloaded, the index went out of range).
    void *(*resolve)(const struct LUA_STRUCT_REF *ref);
    int32_t idx;
    uint32_t gen;
} LUA_STRUCT_REF;

// Creates the type's metatable. `methods` is the full set of C methods the type
// *can* offer; none of them is exposed until a script names it. Terminate with
// {nullptr, nullptr}.
void LUA_Struct_Register(
    lua_State *L, const TYPE_DESC *type, const luaL_Reg *methods);

// Registers trxc.struct, through which Lua declares the public surface.
void LUA_CreateStruct(lua_State *L);

// Push a handle userdata for an instance of a registered type.
void LUA_Struct_Push(
    lua_State *L, const TYPE_DESC *type,
    void *(*resolve)(const LUA_STRUCT_REF *), int32_t idx, uint32_t gen);

// Fetch and typecheck a handle at the given stack index.
LUA_STRUCT_REF *LUA_Struct_CheckRef(
    lua_State *L, int idx, const TYPE_DESC *type);

// Resolve a handle to a live pointer, raising a Lua error if it is stale.
void *LUA_Struct_Deref(lua_State *L, LUA_STRUCT_REF *ref);
