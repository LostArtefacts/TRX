// The string utility surface. The assertions live in
// strings.lua; this stands up the world they run against.

#include <harness/lua_surface.h>

static int M_FakeReset(lua_State *const L)
{
    return 0;
}

static int M_FakeCalls(lua_State *const L)
{
    lua_newtable(L);
    return 1;
}

static void M_PushFake(lua_State *const L)
{
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "strings",
        .tests = "api/strings",
        .push_fake = M_PushFake,
        .fake_reset = M_FakeReset,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
