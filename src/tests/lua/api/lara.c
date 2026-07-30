// The Lara surface. The assertions live in lara.lua; this
// stands up the world they run against.
//
// trx.lara is a module standing for one C struct, so a read of trx.lara.air
// goes through the reflection layer into a real LARA_INFO. That is the thing
// being tested; only what is not a field of it is faked.

#include <fakes/items.h>
#include <fakes/lara.h>
#include <harness/lua_surface.h>

static int M_FakeReset(lua_State *const L)
{
    FakeItems_Reset();
    FakeLara_Reset();
    return 0;
}

static int M_FakeCalls(lua_State *const L)
{
    lua_newtable(L);
    FakeLara_PushCalls(L);
    return 1;
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "lara",
        .deps = { "query", "items", nullptr },
        .tests = "api/lara",
        .fake_reset = M_FakeReset,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
