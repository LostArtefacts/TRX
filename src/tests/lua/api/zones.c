// The zone surface. The assertions live in zones.lua; this stands up the world
// they run against.
//
// Everything under them is real: the spatial query the zones ask, the event
// registry they report through, and src/lua/api/zones.lua itself. Only the item
// world below that is fake. fake.control() fires the after_control event the
// game phase fires, which is what drives a frame of the zones.

#include <fakes/camera.h>
#include <fakes/items.h>
#include <harness/fake_calls.h>
#include <harness/lua_surface.h>

#include <trx/game/camera.h>
#include <trx/game/items.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/events.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>

#include <lauxlib.h>

static int M_L_Control(lua_State *const L)
{
    LUA_FireEvent(LUA_EVENT_AFTER_CONTROL);
    return 0;
}

// What LUA_DropLevelScript does, which the unload of a level reaches: the
// script is told it is being let go of, and then its listeners go.
static void M_DropLevelScript(void)
{
    LUA_FireEvent(LUA_EVENT_LEVEL_UNLOAD);
    LUA_ClearLevelListeners();
}

// fake.end_level() - the level unloading, which is where the zones a script
// made go.
static int M_L_EndLevel(lua_State *const L)
{
    M_DropLevelScript();
    return 0;
}

// fake.game_start() - the level's first frame, which comes after its script has
// run and takes nothing away from it.
static int M_L_GameStart(lua_State *const L)
{
    const LUA_EVENT_ARG args[] = {
        { .type = LUA_EVENT_ARG_INT32, .value = { .i32 = 1 } },
        { .type = LUA_EVENT_ARG_BOOL, .value = { .b = false } },
    };
    LUA_FireEventEx(LUA_EVENT_GAME_START, args, 2);
    return 0;
}

// fake.as_level_script(fn) - attach from a level script rather than a global
// one, so what a level change drops has something to drop.
static int M_L_AsLevelScript(lua_State *const L)
{
    luaL_checktype(L, 1, LUA_TFUNCTION);
    LUA_SetScriptContext(LUA_CONTEXT_LEVEL);
    const int status = lua_pcall(L, 0, 0, 0);
    LUA_SetScriptContext(LUA_CONTEXT_GLOBAL);
    if (status != LUA_OK) {
        return lua_error(L);
    }
    return 0;
}

// fake.destroy(item_num) - the removal path, which fires while the item still
// resolves.
static int M_L_Destroy(lua_State *const L)
{
    const int16_t item_num = (int16_t)luaL_checkinteger(L, 1);
    LUA_FireEventInt32(LUA_EVENT_DESTROY, item_num);
    Item_Destroy(item_num);
    return 0;
}

// fake.flyby({x,y,z}, [room_num]) - a sequence playing, with the camera where
// the call says. fake.flyby() with nothing ends it.
static int M_L_Flyby(lua_State *const L)
{
    if (lua_isnoneornil(L, 1)) {
        FakeCamera_SetFlybyActive(false);
        return 0;
    }
    const XYZ_32 pos = LUA_CheckXYZ(L, 1);
    g_Camera.pos.pos = pos;
    g_Camera.pos.room_num = (int16_t)luaL_optinteger(L, 2, 0);
    FakeCamera_SetFlybyActive(true);
    return 0;
}

// Each case starts with the zones a previous one made gone, which is what the
// unload of a level does to them.
static int M_FakeReset(lua_State *const L)
{
    FakeCalls_Reset();
    LUA_SetScriptContext(LUA_CONTEXT_GLOBAL);
    M_DropLevelScript();
    return 0;
}

static void M_PushFake(lua_State *const L)
{
    lua_pushcfunction(L, M_L_Control);
    lua_setfield(L, -2, "control");
    lua_pushcfunction(L, M_L_EndLevel);
    lua_setfield(L, -2, "end_level");
    lua_pushcfunction(L, M_L_GameStart);
    lua_setfield(L, -2, "game_start");
    lua_pushcfunction(L, M_L_AsLevelScript);
    lua_setfield(L, -2, "as_level_script");
    lua_pushcfunction(L, M_L_Destroy);
    lua_setfield(L, -2, "destroy");
    lua_pushcfunction(L, M_L_Flyby);
    lua_setfield(L, -2, "flyby");
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "zones",
        .deps = { "catalog", "query", "items", "rooms", "camera", "events",
                  "lara", nullptr },
        .tests = "api/zones",
        .push_fake = M_PushFake,
        .fake_reset = M_FakeReset,
    };
    return LuaSurface_Run(&test);
}
