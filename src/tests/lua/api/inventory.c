// What Lara carries. The assertions live in inventory.lua; this stands up the
// world they run against.
//
// The fake keeps a count per object rather than a ring, which is all the
// surface asks of it.

#include <fakes/items.h>
#include <fakes/lara.h>
#include <harness/lua_surface.h>

#include <lauxlib.h>

// fake.set_can_add(bool) - whether the level carries the inventory models.
static int M_FakeSetCanAdd(lua_State *const L)
{
    FakeLara_SetCanAdd(lua_toboolean(L, 1));
    return 0;
}

static int M_FakeSetWeaponAvailable(lua_State *const L)
{
    FakeLara_SetWeaponAvailable(
        (LARA_GUN_TYPE)luaL_checkinteger(L, 1), lua_toboolean(L, 2));
    return 0;
}

static void M_PushFake(lua_State *const L)
{
    lua_pushcfunction(L, M_FakeSetCanAdd);
    lua_setfield(L, -2, "set_can_add");
    lua_pushcfunction(L, M_FakeSetWeaponAvailable);
    lua_setfield(L, -2, "set_weapon_available");
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "inventory",
        .deps = { "query", "items", "catalog", "weapons", nullptr },
        .tests = "api/inventory",
        .push_fake = M_PushFake,
    };
    return LuaSurface_Run(&test);
}
