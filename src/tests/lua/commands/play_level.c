// The /play command, exercised through the console that dispatches it. The game
// flow is the fake's two-level table with a gym, so this says whether the
// command parses a number, a title (spaces and all) or the gym, and routes each
// to the right call. Name matching goes through the pcre2-free fake strings.

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
        .deps = { "log", "game", "locale", "strings", "argparse", nullptr },
        .script = "play_level",
        .tests = "commands/play_level",
        .push_fake = M_PushFake,
    };
    return LuaSurface_Run(&test);
}
