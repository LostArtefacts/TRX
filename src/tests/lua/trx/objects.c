// The object surface. The assertions live in objects.lua;
// this stands up the world they run against.
//
// The object world is the same one the item tests use - an object is what an
// item is cut from, so they share a fake.

#include <fakes/items.h>
#include <harness/lua_surface.h>
#include <trx/game/items.h>
#include <trx/game/objects/common.h>

// Runs the object's control for the item, as the engine does each frame.
static int M_Control(lua_State *const L)
{
    const int16_t item_num = (int16_t)luaL_checkinteger(L, 1);
    const OBJECT *const obj = Object_Get(Item_Get(item_num)->object_id);
    if (obj->control_func != nullptr) {
        obj->control_func(item_num);
    }
    return 0;
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
    lua_pushcfunction(L, M_Control);
    lua_setfield(L, -2, "control");
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "objects",
        // trx.objects.query matches names with trx.strings over the catalog,
        // and is itself built on trx.query, so all three ride along.
        .deps = { "strings", "catalog", "query", nullptr },
        .tests = "trx/objects",
        .push_fake = M_PushFake,
    };
    return LuaSurface_Run(&test);
}
