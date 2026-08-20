#include <fakes/console.h>
#include <fakes/game.h>
#include <harness/fake_calls.h>
#include <harness/lua_surface.h>

static int M_FakeReset(lua_State *const L)
{
    FakeCalls_Reset();
    FakeGame_SetCurrentLevel(0);
    return 0;
}

static void M_PushFake(lua_State *const L)
{
    FakeConsole_PushLua(L);
    FakeGame_PushLua(L);
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .fake_reset = M_FakeReset,
        .module = "console",
        .deps = { "log", "game", "locale", "argparse", nullptr },
        .script = "version",
        .tests = "commands/version",
        .push_fake = M_PushFake,
    };
    return LuaSurface_Run(&test);
}
