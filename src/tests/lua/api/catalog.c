// The catalog surface. The assertions live in catalog.lua;
// this stands up the world they run against.

#include <harness/lua_surface.h>

#define FAKE_SLOT_OFFSET 13

static void M_PushFake(lua_State *const L)
{
    lua_pushinteger(L, FAKE_SLOT_OFFSET);
    lua_setfield(L, -2, "SLOT_OFFSET");
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "catalog",
        .tests = "api/catalog",
        .push_fake = M_PushFake,
    };
    return LuaSurface_Run(&test);
}
