#pragma once

#include <lua.h>

// The wall a level script runs behind. Depends on nothing but Lua, so a unit
// test can stand the same wall up against a bare state.
//
// Not a hard sandbox for hostile code: every script shares one lua_State.

// The standard library, minus the parts a script has no business having.
void LUA_OpenSafeLibs(lua_State *L);

// Closes the escapes the base library leaves open. Takes require and package
// with it, so call it once the trx.* modules have loaded.
void LUA_HardenGlobals(lua_State *L);
