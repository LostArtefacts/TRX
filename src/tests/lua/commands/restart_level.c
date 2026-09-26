// The /restartlevel command, exercised through the console that dispatches it.

#include <fakes/console.h>
#include <fakes/game.h>
#include <harness/lua_surface.h>

static void M_PushFake(lua_State *const L)
{
    FakeConsole_PushLua(L);
    FakeGame_PushLua(L);
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "console",
        .deps = { "log", "game", "locale", "argparse", nullptr },
        .script = "restart_level",
        .tests = "commands/restart_level",
        .push_fake = M_PushFake,
    };
    return LuaSurface_Run(&test);
}
