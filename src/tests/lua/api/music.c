// The music surface. The assertions live in music.lua; this
// stands up the world they run against.

#include <fakes/music.h>
#include <harness/lua_surface.h>

#include <lauxlib.h>

static int M_FakeSetPlaying(lua_State *const L)
{
    FakeMusic_SetPlaying((int32_t)luaL_checkinteger(L, 1));
    return 0;
}

static int M_FakeSetLooped(lua_State *const L)
{
    FakeMusic_SetLooped((int32_t)luaL_checkinteger(L, 1));
    return 0;
}

static int M_FakeSetStream(lua_State *const L)
{
    FakeMusic_SetStream(
        (int32_t)luaL_checkinteger(L, 1), (int32_t)luaL_checkinteger(L, 2),
        (int32_t)luaL_checkinteger(L, 3), luaL_checknumber(L, 4));
    return 0;
}

static void M_PushFake(lua_State *const L)
{
    lua_pushinteger(L, FAKE_MUSIC_TRACK);
    lua_setfield(L, -2, "TRACK");
    lua_pushinteger(L, FAKE_MUSIC_MISSING_TRACK);
    lua_setfield(L, -2, "MISSING_TRACK");
    lua_pushcfunction(L, M_FakeSetPlaying);
    lua_setfield(L, -2, "set_playing");
    lua_pushcfunction(L, M_FakeSetLooped);
    lua_setfield(L, -2, "set_looped");
    lua_pushcfunction(L, M_FakeSetStream);
    lua_setfield(L, -2, "set_stream");
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "music",
        .tests = "api/music",
        .push_fake = M_PushFake,
    };
    return LuaSurface_Run(&test);
}
