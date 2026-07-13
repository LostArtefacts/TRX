// The game flow surface. The assertions live in src/tests/unit/lua/game.lua;
// this stands up the world they run against.

#include "fake_engine_game.h"
#include "lua_surface.h"

#include <lauxlib.h>

extern void LUA_CreateGame(lua_State *L);

static void M_SetUpTRXC(lua_State *const L)
{
    LUA_CreateGame(L);
}

static int M_FakeReset(lua_State *const L)
{
    FakeGame_Reset();
    return 0;
}

static int M_FakeSetCurrentLevel(lua_State *const L)
{
    FakeGame_SetCurrentLevel(
        lua_isnil(L, 1) ? -1 : (int32_t)luaL_checkinteger(L, 1) - 1);
    return 0;
}

static int M_FakeCalls(lua_State *const L)
{
    lua_newtable(L);
    lua_pushinteger(L, g_FakeGameCalls.play_level);
    lua_setfield(L, -2, "play_level");
    lua_pushinteger(L, g_FakeGameCalls.play_cutscene);
    lua_setfield(L, -2, "play_cutscene");
    lua_pushinteger(L, g_FakeGameCalls.play_demo);
    lua_setfield(L, -2, "play_demo");
    lua_pushinteger(L, g_FakeGameCalls.last_num);
    lua_setfield(L, -2, "last_num");
    return 1;
}

static void M_PushFake(lua_State *const L)
{
    lua_pushcfunction(L, M_FakeSetCurrentLevel);
    lua_setfield(L, -2, "set_current_level");
    lua_pushinteger(L, FAKE_LEVEL_COUNT);
    lua_setfield(L, -2, "LEVEL_COUNT");
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "game",
        .tests = "game",
        .setup_trxc = M_SetUpTRXC,
        .push_fake = M_PushFake,
        .fake_reset = M_FakeReset,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
