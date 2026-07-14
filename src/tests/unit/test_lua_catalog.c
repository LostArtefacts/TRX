// The catalog surface. The assertions live in src/tests/unit/lua/catalog.lua;
// this stands up the world they run against.

#include "lua_surface.h"

#define FAKE_SLOT_OFFSET 13

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
    lua_pushinteger(L, FAKE_SLOT_OFFSET);
    lua_setfield(L, -2, "SLOT_OFFSET");
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "catalog",
        .tests = "catalog",
        .push_fake = M_PushFake,
        .fake_reset = M_FakeReset,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
