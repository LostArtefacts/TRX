// The game flow surface. The assertions live in src/tests/unit/lua/game.lua;
// this stands up the world they run against.

#include "fake_engine_game.h"
#include "lua_surface.h"

static int M_FakeReset(lua_State *const L)
{
    FakeGame_Reset();
    return 0;
}

static int M_FakeCalls(lua_State *const L)
{
    lua_newtable(L);
    FakeGame_PushCalls(L);
    return 1;
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "game",
        .tests = "game",
        .push_fake = FakeGame_PushLua,
        .fake_reset = M_FakeReset,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
