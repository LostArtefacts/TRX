// The game flow surface. The assertions live in game.lua;
// this stands up the world they run against.

#include <fakes/game.h>
#include <harness/lua_surface.h>

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
        .tests = "api/game",
        .push_fake = FakeGame_PushLua,
        .fake_reset = M_FakeReset,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
