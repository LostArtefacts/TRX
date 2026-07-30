// Sealing is a one-way door, so it gets a state of its own. Same world as
// items.c; the assertions live in seal.lua.

#include <fakes/items.h>
#include <harness/lua_surface.h>

static void M_SetUpExtra(lua_State *const L)
{
    (void)luaL_dostring(
        L,
        "trx.rooms = setmetatable({}, {\n"
        "  __index = function(_, n) return { num = n } end })\n");
}

static void M_PushFake(lua_State *const L)
{
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "items",
        .deps = { "query", nullptr },
        .tests = "seal",
        .setup_extra = M_SetUpExtra,
        .push_fake = M_PushFake,
    };
    return LuaSurface_Run(&test);
}
