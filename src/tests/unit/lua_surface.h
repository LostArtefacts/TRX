#pragma once

// Runner for the Lua surface tests.
//
// It stands the world up the way common.c does at boot - minus the engine - and
// then hands over to src/tests/unit/lua/<tests>.lua, where the assertions are
// written the way a level script would write them.
//
// A test names the module it exercises and the fake engine's Lua face;
// everything else is the same for all of them.

#include <lauxlib.h>
#include <lualib.h>

typedef struct {
    // src/lua/api/<module>.lua - the declaration under test.
    const char *module;
    // Modules loaded before it, for a declaration that reaches into another -
    // trx.camera.room hands back a trx.rooms.Room. NULL-terminated.
    const char *deps[8];
    // src/lua/commands/<script>.lua, run once the modules are up, the way the
    // engine runs it after sealing. Optional.
    const char *script;
    // Seal the surface and take trxc off the globals, as the engine does once
    // the modules have declared.
    bool seal;
    // Harden the globals, as the engine does last of all.
    bool harden;
    // src/tests/unit/lua/<tests>.lua - the assertions.
    const char *tests;
    // Anything the test needs beyond the bridges, which the linked modules
    // register themselves. Optional.
    void (*setup_extra)(lua_State *L);
    // Adds the fake engine's constants to the `fake` table.
    void (*push_fake)(lua_State *L);
    // fake.reset() and fake.calls().
    lua_CFunction fake_reset;
    lua_CFunction fake_calls;
} LUA_SURFACE_TEST;

// Runs the test file and returns its failure count as a process exit code.
int LuaSurface_Run(const LUA_SURFACE_TEST *test);
