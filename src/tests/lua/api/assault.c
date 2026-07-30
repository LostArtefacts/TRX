// The assault course surface. The assertions live in
// assault.lua; this stands up the world they run against.

#include <fakes/assault.h>
#include <harness/lua_surface.h>

#include <lauxlib.h>

static int M_FakeReset(lua_State *const L)
{
    FakeAssault_Reset();
    return 0;
}

static int M_FakeSetInGym(lua_State *const L)
{
    FakeAssault_SetInGym(lua_toboolean(L, 1));
    return 0;
}

static int M_FakeSetHasStats(lua_State *const L)
{
    FakeAssault_SetHasStats(
        (GYM_TRACK_TYPE)luaL_checkinteger(L, 1), lua_toboolean(L, 2));
    return 0;
}

static int M_FakeSetRunning(lua_State *const L)
{
    FakeAssault_SetRunning(
        (GYM_TRACK_TYPE)luaL_checkinteger(L, 1), lua_toboolean(L, 2));
    return 0;
}

static int M_FakeSetVisible(lua_State *const L)
{
    FakeAssault_SetVisible(
        (GYM_TRACK_TYPE)luaL_checkinteger(L, 1), lua_toboolean(L, 2));
    return 0;
}

static int M_FakeSetActiveTrack(lua_State *const L)
{
    FakeAssault_SetActiveTrack(
        lua_isnil(L, 1) ? GYM_TRACK_NONE
                        : (GYM_TRACK_TYPE)luaL_checkinteger(L, 1));
    return 0;
}

static int M_FakeCalls(lua_State *const L)
{
    lua_newtable(L);
    lua_pushinteger(L, g_FakeAssaultCalls.start);
    lua_setfield(L, -2, "start");
    lua_pushinteger(L, g_FakeAssaultCalls.stop);
    lua_setfield(L, -2, "stop");
    lua_pushinteger(L, g_FakeAssaultCalls.reset);
    lua_setfield(L, -2, "reset");
    lua_pushinteger(L, g_FakeAssaultCalls.finish);
    lua_setfield(L, -2, "finish");
    lua_pushinteger(L, g_FakeAssaultCalls.last_track);
    lua_setfield(L, -2, "last_track");
    lua_pushinteger(L, g_FakeAssaultCalls.config_writes);
    lua_setfield(L, -2, "config_writes");
    return 1;
}

static void M_PushFake(lua_State *const L)
{
    lua_pushcfunction(L, M_FakeSetInGym);
    lua_setfield(L, -2, "set_in_gym");
    lua_pushcfunction(L, M_FakeSetHasStats);
    lua_setfield(L, -2, "set_has_stats");
    lua_pushcfunction(L, M_FakeSetRunning);
    lua_setfield(L, -2, "set_running");
    lua_pushcfunction(L, M_FakeSetVisible);
    lua_setfield(L, -2, "set_visible");
    lua_pushcfunction(L, M_FakeSetActiveTrack);
    lua_setfield(L, -2, "set_active_track");
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "assault",
        .tests = "api/assault",
        .push_fake = M_PushFake,
        .fake_reset = M_FakeReset,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
