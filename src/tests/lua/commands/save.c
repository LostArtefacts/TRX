// /save and /qs, through the console. Needs a level loaded to be playable, so
// the game fake supplies one; the save system fake records the pool and index.

#include <fakes/console.h>
#include <fakes/game.h>
#include <fakes/savegame.h>
#include <harness/lua_surface.h>

static int M_FakeReset(lua_State *const L)
{
    FakeConsole_Reset();
    FakeGame_Reset();
    FakeSavegame_Reset();
    return 0;
}

static int M_FakeCalls(lua_State *const L)
{
    lua_newtable(L);
    FakeConsole_PushCalls(L);
    FakeSavegame_PushCalls(L);
    return 1;
}

static void M_PushFake(lua_State *const L)
{
    FakeConsole_PushLua(L);
    FakeGame_PushLua(L);
    FakeSavegame_PushLua(L);
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "console",
        .deps = { "log", "savegame", "game", "locale", "argparse", nullptr },
        .script = "save_game",
        .tests = "commands/save",
        .push_fake = M_PushFake,
        .fake_reset = M_FakeReset,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
