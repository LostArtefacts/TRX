// The sound surface. The assertions live in sound.lua; this
// stands up the world they run against.

#include <fakes/sound.h>
#include <harness/lua_surface.h>

#include <lauxlib.h>

static int M_FakeSetStream(lua_State *const L)
{
    FakeSound_SetStream(
        (int32_t)luaL_checkinteger(L, 1), (int32_t)luaL_checkinteger(L, 2));
    return 0;
}

static void M_PushFake(lua_State *const L)
{
    lua_pushinteger(L, FAKE_SAMPLE);
    lua_setfield(L, -2, "SAMPLE");
    lua_pushinteger(L, FAKE_MISSING_SAMPLE);
    lua_setfield(L, -2, "MISSING_SAMPLE");
    lua_pushinteger(L, FAKE_SAMPLE_VOLUME);
    lua_setfield(L, -2, "SAMPLE_VOLUME");
    lua_pushcfunction(L, M_FakeSetStream);
    lua_setfield(L, -2, "set_stream");
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "sound",
        .tests = "api/sound",
        .push_fake = M_PushFake,
    };
    return LuaSurface_Run(&test);
}
