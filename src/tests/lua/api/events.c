// The event surface. The assertions live in events.lua; this
// stands up the world they run against.
//
// `fake.fire()` calls the same LUA_FireEvent* entrypoints the engine calls,
// and flip effects go through the same ItemAction interceptor seam that
// floor_data.c and the animation-command paths fire through. That is the point:
// it pins the declared callback arguments against what C pushes.

#include <harness/fake_calls.h>
#include <harness/lua_surface.h>

#include <trx/game/console/common.h>
#include <trx/game/items/actions.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/events.h>
#include <trx/game/lua/registry.h>

#include <lauxlib.h>
#include <string.h>

// What the console was told to show. A handler that raises reports through the
// console, so the count is how the tests see that it did.
static int32_t m_ConsoleShows = 0;

// fake.fire(name, ...) - mirrors the engine's own fire sites, argument for
// argument.
// Drops every listener, then stands the module back up: the shutdown clears
// m_L, and the next fire would be a no-op without it.
static int M_FakeReset(lua_State *const L)
{
    FakeCalls_Reset();
    LUA_Registry_ShutdownAll();
    LUA_Registry_CreateAll(L);
    LUA_SetScriptContext(LUA_CONTEXT_GLOBAL);
    return 0;
}

static int M_FakeFire(lua_State *const L)
{
    const char *const name = luaL_checkstring(L, 1);

    if (strcmp(name, "before_control") == 0) {
        LUA_FireEvent(LUA_EVENT_BEFORE_CONTROL);
    } else if (strcmp(name, "after_control") == 0) {
        LUA_FireEvent(LUA_EVENT_AFTER_CONTROL);
    } else if (strcmp(name, "on_pickup") == 0) {
        LUA_FireEventInt32(LUA_EVENT_PICKUP, luaL_checkinteger(L, 2));
    } else if (strcmp(name, "on_game_start") == 0) {
        LUA_FireEventBool(LUA_EVENT_GAME_START, lua_toboolean(L, 2));
    } else if (strcmp(name, "on_title_start") == 0) {
        LUA_FireEvent(LUA_EVENT_TITLE_START);
    } else if (strcmp(name, "on_flip_effect") == 0) {
        // The seam floor_data.c and the animation-command paths fire through.
        // The result - whether a script took the effect - is handed back, so
        // the tests observe claims the same way the engine does.
        lua_pushboolean(
            L,
            ItemAction_Intercept(
                luaL_checkinteger(L, 2), luaL_checkinteger(L, 3),
                luaL_checkinteger(L, 4)));
        return 1;
    } else {
        return luaL_error(L, "unknown event: %s", name);
    }
    return 0;
}

// fake.as_level_script(fn) - attach from a level script rather than a global
// one, so the level-scoped teardown below has something to clear.
static int M_FakeAsLevelScript(lua_State *const L)
{
    luaL_checktype(L, 1, LUA_TFUNCTION);
    LUA_SetScriptContext(LUA_CONTEXT_LEVEL);
    const int status = lua_pcall(L, 0, 0, 0);
    LUA_SetScriptContext(LUA_CONTEXT_GLOBAL);
    if (status != LUA_OK) {
        return lua_error(L);
    }
    return 0;
}

// fake.end_level() - what the engine does when a level ends.
static int M_FakeEndLevel(lua_State *const L)
{
    LUA_ClearLevelListeners();
    return 0;
}

// fake.console_shows() -> integer
static int M_FakeConsoleShows(lua_State *const L)
{
    lua_pushinteger(L, m_ConsoleShows);
    return 1;
}

static void M_PushFake(lua_State *const L)
{
    lua_pushcfunction(L, M_FakeFire);
    lua_setfield(L, -2, "fire");
    lua_pushcfunction(L, M_FakeAsLevelScript);
    lua_setfield(L, -2, "as_level_script");
    lua_pushcfunction(L, M_FakeEndLevel);
    lua_setfield(L, -2, "end_level");
    lua_pushcfunction(L, M_FakeConsoleShows);
    lua_setfield(L, -2, "console_shows");
}

void Console_LogImpl(
    const LOG_LEVEL level, const char *const file, const int line,
    const char *const func, const char *const fmt, ...)
{
}

void Console_ShowImpl(
    const LOG_LEVEL level, const char *const file, const int line,
    const char *const func, const char *const fmt, ...)
{
    m_ConsoleShows++;
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .fake_reset = M_FakeReset,
        .module = "events",
        .tests = "api/events",
        .push_fake = M_PushFake,
    };
    return LuaSurface_Run(&test);
}
