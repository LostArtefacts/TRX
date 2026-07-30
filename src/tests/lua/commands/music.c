// The /music command, through the console. The fake soundtrack has one track
// (id 5). This pins that the keywords, a track number and an out-of-range
// number each route to the right call.

#include <fakes/console.h>
#include <fakes/music.h>
#include <harness/fake_calls.h>
#include <harness/lua_surface.h>

#include <lauxlib.h>

static void M_PushFake(lua_State *const L)
{
    FakeConsole_PushLua(L);
    lua_pushinteger(L, FAKE_MUSIC_TRACK);
    lua_setfield(L, -2, "TRACK");
    lua_pushinteger(L, FAKE_MUSIC_MISSING_TRACK);
    lua_setfield(L, -2, "MISSING_TRACK");
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "console",
        .deps = { "log", "music", "locale", "strings", "argparse", nullptr },
        .script = "music",
        .tests = "commands/music",
        .push_fake = M_PushFake,
    };
    return LuaSurface_Run(&test);
}
