#include <harness/lua_surface.h>

#include <trx/game/lua/registry.h>
#include <trx/game/lua/sandbox.h>
#include <trx/game/lua/utils.h>

#include <stdio.h>
#include <stdlib.h>

static void M_Fail(lua_State *const L, const char *const what)
{
    fprintf(stderr, "%s: %s\n", what, lua_tostring(L, -1));
    exit(EXIT_FAILURE);
}

// Under the chunk name the engine would give it, not the path it sits at.
// LUA_GetCallerInfo tells the engine's own frames from a script's by that name,
// so loading a module as its path would test a naming the engine never uses.
static void M_RunFileAs(
    lua_State *const L, const char *const path, const char *const chunk_name)
{
    FILE *const fp = fopen(path, "rb");
    if (fp == nullptr) {
        fprintf(stderr, "cannot open %s\n", path);
        exit(EXIT_FAILURE);
    }
    fseek(fp, 0, SEEK_END);
    const long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *const source = malloc((size_t)size);
    if (source == nullptr
        || fread(source, 1, (size_t)size, fp) != (size_t)size) {
        fprintf(stderr, "cannot read %s\n", path);
        exit(EXIT_FAILURE);
    }
    fclose(fp);

    if (luaL_loadbuffer(L, source, (size_t)size, chunk_name) != LUA_OK
        || lua_pcall(L, 0, 0, 0) != LUA_OK) {
        M_Fail(L, path);
    }
    free(source);
}

static void M_RunModule(lua_State *const L, const char *const name)
{
    char path[512];
    char chunk_name[256];
    snprintf(path, sizeof(path), REPO_ROOT "/src/lua/api/%s.lua", name);
    snprintf(
        chunk_name, sizeof(chunk_name), LUA_API_CHUNK_PREFIX "%s.lua", name);
    M_RunFileAs(L, path, chunk_name);
}

int LuaSurface_Run(const LUA_SURFACE_TEST *const test)
{
    lua_State *const L = luaL_newstate();
    luaL_openlibs(L);

    lua_newtable(L);
    lua_setglobal(L, "trxc");
    lua_newtable(L);
    lua_setglobal(L, "trx");

    LUA_Registry_CreateAll(L);
    if (test->setup_extra != nullptr) {
        test->setup_extra(L);
    }

    lua_newtable(L);
    lua_pushcfunction(L, test->fake_reset);
    lua_setfield(L, -2, "reset");
    lua_pushcfunction(L, test->fake_calls);
    lua_setfield(L, -2, "calls");
    if (test->push_fake != nullptr) {
        test->push_fake(L);
    }
    lua_setglobal(L, "fake");

    if (luaL_dostring(
            L,
            "trx.log = { debug = function() end, info = function() end,\n"
            "            warn = function() end, error = function() end }\n"
            "package.preload['trx.log'] = function() return trx.log end\n"
            "package.path = '" REPO_ROOT
            "/src/tests/lua/?.lua;' .. package.path\n"
            // Hardening takes require() away, and a hardened suite still needs
            // the harness.
            "_G.harness = require('harness')\n")
        != LUA_OK) {
        M_Fail(L, "preamble");
    }

    // The declarations are read from the tree, not from the copy embedded in
    // the binary: the test exists to pin what src/lua says today.
    M_RunModule(L, "api");

    const int32_t max_deps = sizeof(test->deps) / sizeof(test->deps[0]);
    for (int32_t i = 0; i < max_deps && test->deps[i] != nullptr; i++) {
        M_RunModule(L, test->deps[i]);

        // A module states its dependencies with require(). It is loaded now, so
        // hand it back.
        lua_pushfstring(
            L, "package.preload['trx.%s'] = function() return trx.%s end",
            test->deps[i], test->deps[i]);
        if (luaL_dostring(L, lua_tostring(L, -1)) != LUA_OK) {
            M_Fail(L, "preloading a dependency");
        }
        lua_pop(L, 1);
    }

    M_RunModule(L, test->module);

    // From here to the tests, the order is LUA_Init's: seal, then the runtime
    // scripts, then harden.
    if (test->seal) {
        if (luaL_dostring(L, "trx.api.seal()") != LUA_OK) {
            M_Fail(L, "sealing");
        }
        lua_pushnil(L);
        lua_setglobal(L, "trxc");
    }

    if (test->script != nullptr) {
        // Not an API module, and not named as one: a command written in Lua is
        // blamed for its own log lines.
        char path[512];
        char chunk_name[256];
        snprintf(
            path, sizeof(path), REPO_ROOT "/src/lua/commands/%s.lua",
            test->script);
        snprintf(
            chunk_name, sizeof(chunk_name), "@trx/commands/%s.lua",
            test->script);
        M_RunFileAs(L, path, chunk_name);
    }

    if (test->harden) {
        LUA_HardenGlobals(L);
    }

    lua_pushfstring(L, REPO_ROOT "/src/tests/lua/%s.lua", test->tests);
    const int32_t base = lua_gettop(L);
    if (luaL_dofile(L, lua_tostring(L, -1)) != LUA_OK) {
        M_Fail(L, "running the tests");
    }

    // A suite that returns nothing has registered its cases and run none of
    // them, and would otherwise read as zero failures.
    if (lua_gettop(L) <= base || !lua_isinteger(L, -1)) {
        M_Fail(L, "the tests returned no failure count - missing h.report()?");
    }

    const int failures = (int)lua_tointeger(L, -1);
    // The engine's own shutdown order: the capi modules let go of the state
    // before it closes, so a destructor holding a ref cannot reach into a
    // closed state.
    LUA_Registry_ShutdownAll();
    lua_close(L);
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
