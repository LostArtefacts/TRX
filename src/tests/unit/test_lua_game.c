// The game flow surface. The assertions live in src/tests/unit/lua/game.lua;
// this stands up the world they run against.

#include "fake_engine_game.h"
#include "lua_surface.h"

#include <lauxlib.h>

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

static int M_FakeSetCurrentTitle(lua_State *const L)
{
    FakeGame_SetCurrentTitle();
    return 0;
}

static int M_FakeSetGymPresent(lua_State *const L)
{
    FakeGame_SetGymPresent(lua_toboolean(L, 1));
    return 0;
}

static int M_FakeSetInCutscene(lua_State *const L)
{
    FakeGame_SetInCutscene(lua_toboolean(L, 1));
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
    lua_pushinteger(L, g_FakeGameCalls.play_gym);
    lua_setfield(L, -2, "play_gym");
    lua_pushinteger(L, g_FakeGameCalls.last_num);
    lua_setfield(L, -2, "last_num");
    return 1;
}

static void M_PushFake(lua_State *const L)
{
    lua_pushcfunction(L, M_FakeSetCurrentLevel);
    lua_setfield(L, -2, "set_current_level");
    lua_pushcfunction(L, M_FakeSetCurrentTitle);
    lua_setfield(L, -2, "set_current_title");
    lua_pushcfunction(L, M_FakeSetGymPresent);
    lua_setfield(L, -2, "set_gym_present");
    lua_pushinteger(L, FAKE_LEVEL_COUNT);
    lua_setfield(L, -2, "LEVEL_COUNT");
    // The levels the game numbers: the gym is in the table but is not one.
    lua_pushinteger(L, FAKE_LEVEL_COUNT - 1);
    lua_setfield(L, -2, "NUMBERED_LEVEL_COUNT");
    lua_pushcfunction(L, M_FakeSetInCutscene);
    lua_setfield(L, -2, "set_in_cutscene");
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "game",
        .tests = "game",
        .push_fake = M_PushFake,
        .fake_reset = M_FakeReset,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
