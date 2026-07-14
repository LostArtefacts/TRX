// The creature surface. The assertions live in
// src/tests/unit/lua/creatures.lua; this stands up the world they run against.

#include "fake_engine_creatures.h"
#include "lua_surface.h"

static int M_FakeReset(lua_State *const L)
{
    FakeCreatures_Reset();
    return 0;
}

static int M_FakeCalls(lua_State *const L)
{
    lua_newtable(L);
    lua_pushinteger(L, g_FakeCreatureCalls.add_ally);
    lua_setfield(L, -2, "add_ally");
    lua_pushinteger(L, g_FakeCreatureCalls.add_ally_target);
    lua_setfield(L, -2, "add_ally_target");
    lua_pushinteger(L, g_FakeCreatureCalls.last_ally_object_id);
    lua_setfield(L, -2, "last_ally_object_id");
    lua_pushinteger(L, g_FakeCreatureCalls.last_ally_target_object_id);
    lua_setfield(L, -2, "last_ally_target_object_id");
    return 1;
}

static void M_PushFake(lua_State *const L)
{
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "creatures",
        .tests = "creatures",
        .push_fake = M_PushFake,
        .fake_reset = M_FakeReset,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
