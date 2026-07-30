// /load and /ql, through the console. The fake save system has ten taken slots
// in each pool, so this pins that a number, `q`, `q2` and `quick` reach the
// right pool and index - the slot spelling is read by a match matcher.

#include <fakes/console.h>
#include <fakes/savegame.h>
#include <harness/lua_surface.h>

static void M_PushFake(lua_State *const L)
{
    FakeConsole_PushLua(L);
    FakeSavegame_PushLua(L);
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "console",
        .deps = { "log", "savegame", "locale", "argparse", nullptr },
        .script = "load_game",
        .tests = "commands/load_game",
        .push_fake = M_PushFake,
    };
    return LuaSurface_Run(&test);
}
