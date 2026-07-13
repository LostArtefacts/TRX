// The event surface. The assertions live in src/tests/unit/lua/events.lua; this
// stands up the world they run against.
//
// There is no fake engine here: lua/events.c is self-contained - attach, detach
// and fire - so `fake.fire()` calls the same Lua_FireEvent* entrypoints the
// engine calls. That is the point: it pins the declared callback arguments
// against what C actually pushes.

#include "lua_surface.h"

#include <trx/game/lua/common.h>
#include <trx/game/lua/events.h>

#include <lauxlib.h>
#include <string.h>

extern void LUA_CreateEvents(lua_State *L);

static LUA_CONTEXT m_Context = LUA_CONTEXT_GLOBAL;

LUA_CONTEXT Lua_GetScriptContext(void)
{
    return m_Context;
}

void Lua_SetScriptContext(const LUA_CONTEXT context)
{
    m_Context = context;
}

static void M_SetUpTRXC(lua_State *const L)
{
    LUA_CreateEvents(L);
}

// fake.fire(name, ...) - mirrors the engine's own fire sites, argument for
// argument. A handler must see exactly what C pushes.
static int M_FakeFire(lua_State *const L)
{
    const char *const name = luaL_checkstring(L, 1);

    if (strcmp(name, "before_control") == 0) {
        Lua_FireEvent(LUA_EVENT_BEFORE_CONTROL);
    } else if (strcmp(name, "after_control") == 0) {
        Lua_FireEvent(LUA_EVENT_AFTER_CONTROL);
    } else if (strcmp(name, "on_pickup") == 0) {
        Lua_FireEventInt32(LUA_EVENT_PICKUP, luaL_checkinteger(L, 2));
    } else if (strcmp(name, "on_game_start") == 0) {
        const LUA_EVENT_ARG args[] = {
            { .type = LUA_EVENT_ARG_INT32,
              .value = { .i32 = luaL_checkinteger(L, 2) } },
            { .type = LUA_EVENT_ARG_BOOL,
              .value = { .b = lua_toboolean(L, 3) } },
        };
        Lua_FireEventEx(LUA_EVENT_GAME_START, args, 2);
    } else if (strcmp(name, "before_level_file") == 0) {
        Lua_FireEventInt32(
            LUA_EVENT_BEFORE_LEVEL_FILE, luaL_checkinteger(L, 2));
    } else if (strcmp(name, "after_level_file") == 0) {
        Lua_FireEventInt32(LUA_EVENT_AFTER_LEVEL_FILE, luaL_checkinteger(L, 2));
    } else if (strcmp(name, "before_item_setup") == 0) {
        Lua_FireEventInt32(
            LUA_EVENT_BEFORE_ITEM_SETUP, luaL_checkinteger(L, 2));
    } else if (strcmp(name, "after_item_setup") == 0) {
        Lua_FireEventInt32(LUA_EVENT_AFTER_ITEM_SETUP, luaL_checkinteger(L, 2));
    } else if (strcmp(name, "after_level_state") == 0) {
        Lua_FireEventInt32(
            LUA_EVENT_AFTER_LEVEL_STATE, luaL_checkinteger(L, 2));
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
    m_Context = LUA_CONTEXT_LEVEL;
    const int status = lua_pcall(L, 0, 0, 0);
    m_Context = LUA_CONTEXT_GLOBAL;
    if (status != LUA_OK) {
        return lua_error(L);
    }
    return 0;
}

// fake.end_level() - what the engine does when a level ends.
static int M_FakeEndLevel(lua_State *const L)
{
    Lua_ClearLevelListeners();
    return 0;
}

static int M_FakeReset(lua_State *const L)
{
    // Drops every listener, then puts m_L back: Lua_ShutdownEvents clears it,
    // and the next fire would be a no-op without it.
    Lua_ShutdownEvents();
    LUA_CreateEvents(L);
    m_Context = LUA_CONTEXT_GLOBAL;
    return 0;
}

static int M_FakeCalls(lua_State *const L)
{
    lua_newtable(L);
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
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "events",
        .tests = "events",
        .setup_trxc = M_SetUpTRXC,
        .push_fake = M_PushFake,
        .fake_reset = M_FakeReset,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
