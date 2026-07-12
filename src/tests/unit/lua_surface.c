#include "lua_surface.h"

#include <stdio.h>
#include <stdlib.h>

extern void LUA_CreateStruct(lua_State *L);
extern void LUA_CreateEnum(lua_State *L);

static void M_Fail(lua_State *const L, const char *const what)
{
    fprintf(stderr, "%s: %s\n", what, lua_tostring(L, -1));
    exit(EXIT_FAILURE);
}

int LuaSurface_Run(const LUA_SURFACE_TEST *const test)
{
    lua_State *const L = luaL_newstate();
    luaL_openlibs(L);

    lua_newtable(L);
    lua_setglobal(L, "trxc");
    lua_newtable(L);
    lua_setglobal(L, "trx");

    LUA_CreateStruct(L);
    LUA_CreateEnum(L);
    test->setup_trxc(L);

    lua_newtable(L);
    lua_pushcfunction(L, test->fake_reset);
    lua_setfield(L, -2, "reset");
    lua_pushcfunction(L, test->fake_calls);
    lua_setfield(L, -2, "calls");
    test->push_fake(L);
    lua_setglobal(L, "fake");

    if (luaL_dostring(
            L,
            "trx.log = { debug = function() end, info = function() end,\n"
            "            warn = function() end, error = function() end }\n"
            "package.preload['trx.log'] = function() return trx.log end\n"
            "package.path = '" REPO_ROOT
            "/src/tests/unit/lua/?.lua;' .. package.path\n")
        != LUA_OK) {
        M_Fail(L, "preamble");
    }

    // The declarations are read from the tree, not from the copy embedded in
    // the binary: the test exists to pin what data/scripting says today.
    if (luaL_dofile(L, REPO_ROOT "/data/scripting/api.lua") != LUA_OK) {
        M_Fail(L, "loading api.lua");
    }
    lua_pushfstring(L, REPO_ROOT "/data/scripting/%s.lua", test->module);
    if (luaL_dofile(L, lua_tostring(L, -1)) != LUA_OK) {
        M_Fail(L, "loading the module");
    }
    lua_pop(L, 1);

    lua_pushfstring(L, REPO_ROOT "/src/tests/unit/lua/%s.lua", test->tests);
    if (luaL_dofile(L, lua_tostring(L, -1)) != LUA_OK) {
        M_Fail(L, "running the tests");
    }

    const int failures = (int)lua_tointeger(L, -1);
    lua_close(L);
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
