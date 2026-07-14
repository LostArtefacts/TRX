// The Lara surface. The assertions live in src/tests/unit/lua/lara.lua; this
// stands up the world they run against.
//
// trx.lara is a module standing for one C struct, so a read of trx.lara.air
// goes through the reflection layer into a real LARA_INFO. That is the thing
// being tested; only what is not a field of it is faked.

#include "fake_engine_items.h"
#include "fake_engine_lara.h"
#include "lua_surface.h"

static int M_FakeReset(lua_State *const L)
{
    FakeItems_Reset();
    FakeLara_Reset();
    return 0;
}

static int M_FakeCalls(lua_State *const L)
{
    lua_newtable(L);
    lua_pushinteger(L, g_FakeLaraCalls.set_equipment);
    lua_setfield(L, -2, "set_equipment");
    lua_pushinteger(L, g_FakeLaraCalls.clear_equipment);
    lua_setfield(L, -2, "clear_equipment");
    lua_pushinteger(L, g_FakeLaraCalls.last_mesh);
    lua_setfield(L, -2, "last_mesh");
    lua_pushinteger(L, g_FakeLaraCalls.last_extra_mesh);
    lua_setfield(L, -2, "last_extra_mesh");
    return 1;
}

static void M_PushFake(lua_State *const L)
{
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "lara",
        .deps = { "items", nullptr },
        .tests = "lara",
        .push_fake = M_PushFake,
        .fake_reset = M_FakeReset,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
