// The object surface. The assertions live in src/tests/unit/lua/objects.lua;
// this stands up the world they run against.
//
// The object world is the same one the item tests use - an object is what an
// item is cut from, so they share a fake.

#include "fake_engine_items.h"
#include "lua_surface.h"

static int M_FakeReset(lua_State *const L)
{
    FakeItems_Reset();
    return 0;
}

static int M_FakeCalls(lua_State *const L)
{
    lua_newtable(L);
    lua_pushinteger(L, g_FakeItemCalls.swap_mesh);
    lua_setfield(L, -2, "swap_mesh");
    return 1;
}

static void M_PushFake(lua_State *const L)
{
    lua_pushinteger(L, FAKE_OBJ_WOLF);
    lua_setfield(L, -2, "WOLF");
    lua_pushinteger(L, FAKE_OBJ_VASE);
    lua_setfield(L, -2, "VASE");
    lua_pushinteger(L, FAKE_OBJ_UNLOADED);
    lua_setfield(L, -2, "UNLOADED");
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "objects",
        .tests = "objects",
        .push_fake = M_PushFake,
        .fake_reset = M_FakeReset,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
