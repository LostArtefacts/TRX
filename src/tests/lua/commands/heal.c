#include <fakes/console.h>
#include <fakes/game.h>
#include <fakes/items.h>
#include <fakes/lara.h>
#include <harness/lua_surface.h>

static int M_FakeReset(lua_State *const L)
{
    FakeConsole_Reset();
    FakeItems_Reset();
    FakeLara_Reset();
    FakeGame_Reset();
    // A level is up unless a test says otherwise: that is where the command has
    // work to do.
    FakeGame_SetCurrentLevel(0);
    return 0;
}

static int M_FakeCalls(lua_State *const L)
{
    lua_newtable(L);
    FakeConsole_PushCalls(L);
    FakeLara_PushCalls(L);
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
        .deps = { "log", "query", "items", "lara", "game", "locale", "argparse",
                  nullptr },
        .script = "heal",
        .tests = "commands/heal",
        .push_fake = M_PushFake,
        .fake_reset = M_FakeReset,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
