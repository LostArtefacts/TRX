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
    lua_pushinteger(L, g_FakeItemCalls.swap_sprite);
    lua_setfield(L, -2, "swap_sprite");
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
    lua_pushinteger(L, FAKE_OBJ_KEY);
    lua_setfield(L, -2, "KEY");
    lua_pushinteger(L, FAKE_OBJ_SWITCH);
    lua_setfield(L, -2, "SWITCH");
    lua_pushinteger(L, FAKE_OBJ_RECEPTACLE);
    lua_setfield(L, -2, "RECEPTACLE");
    lua_pushinteger(L, FAKE_OBJ_DOOR);
    lua_setfield(L, -2, "DOOR");
    lua_pushinteger(L, FAKE_OBJ_SPRITE);
    lua_setfield(L, -2, "SPRITE");
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "objects",
        // trx.objects.query matches names with trx.strings over the catalog,
        // and is itself built on trx.query, so all three ride along.
        .deps = { "strings", "catalog", "query", nullptr },
        .tests = "objects",
        .push_fake = M_PushFake,
        .fake_reset = M_FakeReset,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
