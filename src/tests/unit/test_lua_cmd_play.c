// The /play command, exercised through the console that dispatches it. The game
// flow is the fake's two-level table with a gym, so this says whether the
// command parses a number, a title (spaces and all) or the gym, and routes each
// to the right call. Name matching goes through the pcre2-free fake strings.

#include "fake_engine_console.h"
#include "fake_engine_game.h"
#include "lua_surface.h"

static int M_FakeReset(lua_State *const L)
{
    FakeConsole_Reset();
    FakeGame_Reset();
    return 0;
}

static int M_FakeCalls(lua_State *const L)
{
    lua_newtable(L);
    FakeConsole_PushCalls(L);
    FakeGame_PushCalls(L);
    return 1;
}

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
        .tests = "cmd_play",
        .push_fake = M_PushFake,
        .fake_reset = M_FakeReset,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
