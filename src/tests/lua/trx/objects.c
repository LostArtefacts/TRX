// The object surface. The assertions live in objects.lua;
// this stands up the world they run against.
//
// The object world is the same one the item tests use - an object is what an
// item is cut from, so they share a fake.

#include <fakes/items.h>
#include <harness/lua_surface.h>
#include <trx/game/items.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/registry.h>
#include <trx/game/objects/common.h>

// What a bullet hit on the item spawns, as the engine asks it.
static int M_HitEffect(lua_State *const L)
{
    const ITEM *const item = Item_Get((int16_t)luaL_checkinteger(L, 1));
    const OBJECT *const obj = Object_Get(item->object_id);
    lua_pushinteger(
        L,
        obj->get_hit_effect_func != nullptr ? obj->get_hit_effect_func(item)
                                            : ITEM_HIT_BLOOD);
    return 1;
}

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

// fake.set_level_script(on) - runs what follows as a level script would.
static int M_SetLevelScript(lua_State *const L)
{
    LUA_SetScriptContext(
        lua_toboolean(L, 1) ? LUA_CONTEXT_LEVEL : LUA_CONTEXT_GLOBAL);
    return 0;
}

// fake.end_level() - what the engine does when a level ends.
static int M_EndLevel(lua_State *const L)
{
    LUA_Registry_DropLevelAll();
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
    lua_pushcfunction(L, M_HitEffect);
    lua_setfield(L, -2, "hit_effect");
    lua_pushcfunction(L, M_Control);
    lua_setfield(L, -2, "control");
    lua_pushcfunction(L, M_SetLevelScript);
    lua_setfield(L, -2, "set_level_script");
    lua_pushcfunction(L, M_EndLevel);
    lua_setfield(L, -2, "end_level");
    lua_pushinteger(L, ITEM_HIT_DEFAULT);
    lua_setfield(L, -2, "HIT_DEFAULT");
    lua_pushinteger(L, ITEM_HIT_BLOOD);
    lua_setfield(L, -2, "HIT_BLOOD");
    lua_pushinteger(L, ITEM_HIT_RICOCHET);
    lua_setfield(L, -2, "HIT_RICOCHET");
    lua_pushinteger(L, ITEM_HIT_SMOKE);
    lua_setfield(L, -2, "HIT_SMOKE");
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
