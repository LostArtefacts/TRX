// The /weather command, through the console. The assertions live in
// weather.lua.

#include <fakes/console.h>
#include <fakes/game.h>
#include <harness/fake_calls.h>
#include <harness/lua_surface.h>

static int M_FakeReset(lua_State *const L)
{
    FakeCalls_Reset();
    // A level is up unless a test says otherwise: that is where the command has
    // work to do.
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
        .deps = { "log", "weather", "game", "locale", "strings", "argparse",
                  nullptr },
        .script = "weather",
        .tests = "commands/weather",
        .push_fake = M_PushFake,
    };
    return LuaSurface_Run(&test);
}
