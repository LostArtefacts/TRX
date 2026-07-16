#pragma once

#include <trx/game/lua/field.h>
#include <trx/game/objects.h>

#include <lauxlib.h>
#include <lualib.h>

// Generic Lua bridge over FIELD_DESC.
//
// Registering a type creates its metatable with EMPTY public tables: no field,
// method or computed member is reachable until a script declares it (see
// trx.api.type in src/lua/api.lua). The declaration is what populates the
// metatable, so the public API is coined in Lua, while dispatch stays in C
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

// Push a handle userdata for an instance of a registered type.
void LUA_Struct_Push(
    lua_State *L, const TYPE_DESC *type,
    void *(*resolve)(const LUA_STRUCT_REF *), int32_t idx, uint32_t gen);

// Fetch and typecheck a handle at the given stack index.
LUA_STRUCT_REF *LUA_Struct_CheckRef(
    lua_State *L, int idx, const TYPE_DESC *type);

// Resolve a handle to a live pointer, raising a Lua error if it is stale.
void *LUA_Struct_Deref(lua_State *L, LUA_STRUCT_REF *ref);

// The object property overlay, which items and objects both carry. Fields
// address the C struct; a property is what the object declares about itself,
// plus - for an item - that item's own override. Only the struct the overlay
// hangs off differs between the two, so the three bridges are written once and
// each type says how to reach its own.
typedef struct {
    const TYPE_DESC *type;
    // What an unknown property is called back to the script.
    const char *what;
    bool (*get)(const void *self, const char *name, OBJECT_PROPERTY_VALUE *out);
    bool (*set)(void *self, const char *name, OBJECT_PROPERTY_VALUE value);
    int32_t (*name_count)(const void *self);
    const char *(*name_at)(const void *self, int32_t idx);
} LUA_PROPERTY_DESC;

// Installs get_property/set_property/get_property_names on the type's method
// set, bound to `desc`. Call after LUA_Struct_Register; a declaration exposes
// them by name.
void LUA_Property_Register(lua_State *L, const LUA_PROPERTY_DESC *desc);
