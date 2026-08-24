#include <harness/lua_surface.h>

#include <harness/fake_calls.h>
#include <trx/core/subsystem.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/sandbox.h>
#include <trx/game/lua/utils.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The modules this run loads, in the order their bodies are run: what a module
// requires comes before it. The engine registers every module's preload before
// running any body, so a require during a body hands back a table rather than
// running a second module mid-flight; the harness does the same.
#define M_MAX_MODULES 64
static char *m_Order[M_MAX_MODULES];
static int32_t m_Count;

// Names the walk has been down, which is not the order they run in: a module is
// recorded here on the way in and lands in m_Order on the way out, after what
// it requires.
static char *m_Visited[M_MAX_MODULES];
static int32_t m_VisitedCount;

// The shared fake.reset(), for a fake that records through FAKE_RECORD and
// registers what else it holds with FAKE_ON_RESET.
static int M_Reset(lua_State *const L)
{
    FakeCalls_Reset();
    return 0;
}

static void M_Fail(lua_State *const L, const char *const what)
{
    fprintf(stderr, "%s: %s\n", what, lua_tostring(L, -1));
    exit(EXIT_FAILURE);
}

// Under the chunk name the engine would give it, not the path it sits at.
// LUA_GetCallerInfo tells the engine's own frames from a script's by that name,
// so loading a module as its path would test a naming the engine never uses.
// The file, or exit saying which one could not be read. The caller owns what
// comes back and its length lands in out_size.
static char *M_ReadFile(const char *const path, size_t *const out_size)
{
    FILE *const fp = fopen(path, "rb");
    if (fp == nullptr) {
        fprintf(stderr, "cannot open %s\n", path);
        exit(EXIT_FAILURE);
    }
    fseek(fp, 0, SEEK_END);
    const long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *const source = malloc((size_t)size + 1);
    if (source == nullptr
        || fread(source, 1, (size_t)size, fp) != (size_t)size) {
        fprintf(stderr, "cannot read %s\n", path);
        exit(EXIT_FAILURE);
    }
    source[size] = '\0';
    fclose(fp);
    *out_size = (size_t)size;
    return source;
}

static void M_RunFileAs(
    lua_State *const L, const char *const path, const char *const chunk_name)
{
    size_t size;
    char *const source = M_ReadFile(path, &size);
    if (luaL_loadbuffer(L, source, size, chunk_name) != LUA_OK
        || lua_pcall(L, 0, 0, 0) != LUA_OK) {
        M_Fail(L, path);
    }
    free(source);
}

static bool M_Visit(const char *const name)
{
    for (int32_t i = 0; i < m_VisitedCount; i++) {
        if (strcmp(m_Visited[i], name) == 0) {
            return true;
        }
    }
    if (m_VisitedCount >= M_MAX_MODULES) {
        fprintf(stderr, "more than %d modules to load\n", M_MAX_MODULES);
        exit(EXIT_FAILURE);
    }
    m_Visited[m_VisitedCount++] = strdup(name);
    return false;
}

static void M_Discover(lua_State *L, const char *name, bool forced);

// Whether something already answers for the module - the preamble stubs
// trx.log, and a module requiring it means that stub unless a test asked for
// the real one by naming it.
static bool M_IsProvided(lua_State *const L, const char *const name)
{
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "preload");
    lua_pushfstring(L, "trx.%s", name);
    const bool present = lua_gettable(L, -2) != LUA_TNIL;
    lua_pop(L, 3);
    return present;
}

// A module says what it needs with require("trx.x"), so the harness reads it
// off the source rather than making every test list what its dependencies
// depend on. Nothing here parses Lua: a require inside a comment or a string
// costs one module loaded for nothing, which is harmless.
static void M_DiscoverRequired(lua_State *const L, const char *const source)
{
    static const char *const marker = "require(";
    for (const char *at = strstr(source, marker); at != nullptr;
         at = strstr(at + 1, marker)) {
        const char *cursor = at + strlen(marker);
        while (*cursor == ' ' || *cursor == '"' || *cursor == '\'') {
            cursor++;
        }
        if (strncmp(cursor, "trx.", 4) != 0) {
            continue;
        }
        cursor += 4;

        char name[64];
        size_t len = 0;
        while ((isalnum((unsigned char)*cursor) || *cursor == '_')
               && len + 1 < sizeof(name)) {
            name[len++] = *cursor++;
        }
        name[len] = '\0';
        if (len > 0) {
            M_Discover(L, name, false);
        }
    }
}

// Records the module and everything it requires, dependencies first. A test
// names what it wants for real; everything else is taken only where nothing
// answers for it yet.
// Converts a require name to a module path.
static void M_PathFromName(
    char *const out, const size_t size, const char *const name)
{
    snprintf(out, size, REPO_ROOT "/src/lua/api/%s.lua", name);
    for (char *c = out + strlen(REPO_ROOT "/src/lua/api/"); *c != '\0'; c++) {
        if (*c == '.') {
            *c = '/';
        }
    }
    // Restore the .lua suffix after the slash conversion.
    const size_t len = strlen(out);
    if (len > 4) {
        out[len - 4] = '.';
    }
}

static void M_Discover(
    lua_State *const L, const char *const name, const bool forced)
{
    if ((!forced && M_IsProvided(L, name)) || M_Visit(name)) {
        return;
    }

    char path[512];
    M_PathFromName(path, sizeof(path), name);
    size_t size;
    char *const source = M_ReadFile(path, &size);
    M_DiscoverRequired(L, source);
    free(source);

    // On the way out, so what this module requires is already in front of it.
    // The name is copied: a scanned one points into a source buffer freed
    // above.
    m_Order[m_Count++] = strdup(name);
}

// Runs a module under the chunk name the engine would give it.
// LUA_GetCallerInfo tells the engine's own frames from a script's by that name,
// so loading a module as its path would test a naming the engine never uses.
static void M_RunModule(lua_State *const L, const char *const name)
{
    char path[512];
    char chunk_name[256];
    M_PathFromName(path, sizeof(path), name);
    snprintf(
        chunk_name, sizeof(chunk_name), LUA_API_CHUNK_PREFIX "%s.lua", name);
    M_RunFileAs(L, path, chunk_name);
}

// Every module's preload first, then the bodies in order, as LUA_Init does.
static void M_LoadModules(lua_State *const L)
{
    for (int32_t i = 0; i < m_Count; i++) {
        lua_pushfstring(
            L, "package.preload['trx.%s'] = function() return trx.%s end",
            m_Order[i], m_Order[i]);
        if (luaL_dostring(L, lua_tostring(L, -1)) != LUA_OK) {
            M_Fail(L, "preloading a module");
        }
        lua_pop(L, 1);
    }
    for (int32_t i = 0; i < m_Count; i++) {
        M_RunModule(L, m_Order[i]);
    }
    for (int32_t i = 0; i < m_Count; i++) {
        free(m_Order[i]);
    }
    for (int32_t i = 0; i < m_VisitedCount; i++) {
        free(m_Visited[i]);
    }
    m_Count = 0;
    m_VisitedCount = 0;
}

int LuaSurface_Run(const LUA_SURFACE_TEST *const test)
{
    lua_State *const L = luaL_newstate();
    luaL_openlibs(L);

    lua_newtable(L);
    lua_setglobal(L, "trxc");
    lua_newtable(L);
    lua_setglobal(L, "trx");

    // The bridges subscribe to the modules they wrap as they are created, so
    // the modules stand first.
    Subsystem_InitAll();
    LUA_Registry_CreateAll(L);
    if (test->setup_extra != nullptr) {
        test->setup_extra(L);
    }

    lua_newtable(L);
    lua_pushcfunction(
        L, test->fake_reset != nullptr ? test->fake_reset : M_Reset);
    lua_setfield(L, -2, "reset");
    lua_pushcfunction(
        L, test->fake_calls != nullptr ? test->fake_calls : FakeCalls_Push);
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
    M_Discover(L, "api", true);

    // A full list is a list that was about to be cut short. Saying so beats a
    // module quietly going missing and Lua answering with its search path.
    const int32_t max_deps = sizeof(test->deps) / sizeof(test->deps[0]);
    if (test->deps[max_deps - 1] != nullptr) {
        fprintf(
            stderr,
            "the dependency list is full (%d), so it may have been cut short\n",
            max_deps);
        exit(EXIT_FAILURE);
    }
    for (int32_t i = 0; i < max_deps && test->deps[i] != nullptr; i++) {
        M_Discover(L, test->deps[i], true);
    }

    M_Discover(L, test->module, true);
    M_LoadModules(L);

    // From here to the tests, the order is LUA_Init's: seal, then the runtime
    // scripts, then harden.
    if (test->seal) {
        if (luaL_dostring(L, "trx.api.seal({ partial = true })") != LUA_OK) {
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

    if (test->mod_script != nullptr) {
        // Use the same chunk name as the engine require path.
        char path[512];
        char chunk_name[sizeof(path) + 1];
        snprintf(
            path, sizeof(path), REPO_ROOT "/data/trx/ship/modules/%s.lua",
            test->mod_script);
        snprintf(chunk_name, sizeof(chunk_name), "@%s", path);
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
    Subsystem_ShutdownAll();
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
