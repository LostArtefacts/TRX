// The music surface. The assertions live in src/tests/unit/lua/music.lua; this
// stands up the world they run against.

#include "fake_engine_music.h"
#include "lua_surface.h"

#include <lauxlib.h>

extern void LUA_CreateMusic(lua_State *L);

static void M_SetUpTRXC(lua_State *const L)
{
    LUA_CreateMusic(L);
}

static int M_FakeReset(lua_State *const L)
{
    FakeMusic_Reset();
    return 0;
}

static int M_FakeSetPlaying(lua_State *const L)
{
    FakeMusic_SetPlaying((int32_t)luaL_checkinteger(L, 1));
    return 0;
}

static int M_FakeCalls(lua_State *const L)
{
    lua_newtable(L);
    lua_pushinteger(L, g_FakeMusicCalls.play_count);
    lua_setfield(L, -2, "play_count");
    lua_pushinteger(L, g_FakeMusicCalls.last_track);
    lua_setfield(L, -2, "last_track");
    lua_pushinteger(L, g_FakeMusicCalls.last_mode);
    lua_setfield(L, -2, "last_mode");
    lua_pushinteger(L, g_FakeMusicCalls.pause_count);
    lua_setfield(L, -2, "pause_count");
    lua_pushinteger(L, g_FakeMusicCalls.unpause_count);
    lua_setfield(L, -2, "unpause_count");
    lua_pushinteger(L, g_FakeMusicCalls.stop_count);
    lua_setfield(L, -2, "stop_count");
    return 1;
}

static void M_PushFake(lua_State *const L)
{
    lua_pushinteger(L, FAKE_MUSIC_TRACK);
    lua_setfield(L, -2, "TRACK");
    lua_pushinteger(L, FAKE_MUSIC_MISSING_TRACK);
    lua_setfield(L, -2, "MISSING_TRACK");
    lua_pushcfunction(L, M_FakeSetPlaying);
    lua_setfield(L, -2, "set_playing");
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "music",
        .tests = "music",
        .setup_trxc = M_SetUpTRXC,
        .push_fake = M_PushFake,
        .fake_reset = M_FakeReset,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
