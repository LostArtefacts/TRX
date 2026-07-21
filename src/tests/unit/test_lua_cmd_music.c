// The /music command, through the console. The fake soundtrack has one track
// (id 5). This pins that the keywords, a track number and an out-of-range
// number each route to the right call.

#include "fake_engine_console.h"
#include "fake_engine_music.h"
#include "lua_surface.h"

#include <lauxlib.h>

static int M_FakeReset(lua_State *const L)
{
    FakeConsole_Reset();
    FakeMusic_Reset();
    return 0;
}

static int M_FakeCalls(lua_State *const L)
{
    lua_newtable(L);
    FakeConsole_PushCalls(L);
    lua_pushinteger(L, g_FakeMusicCalls.play_count);
    lua_setfield(L, -2, "play_count");
    lua_pushinteger(L, g_FakeMusicCalls.last_track);
    lua_setfield(L, -2, "last_track");
    lua_pushinteger(L, g_FakeMusicCalls.stop_count);
    lua_setfield(L, -2, "stop_count");
    return 1;
}

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
        .tests = "cmd_music",
        .push_fake = M_PushFake,
        .fake_reset = M_FakeReset,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
