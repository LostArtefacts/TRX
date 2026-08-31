// The room surface. The assertions live in rooms.lua; this
// stands up the world they run against.

#include <fakes/items.h>
#include <fakes/level.h>
#include <fakes/rooms.h>
#include <harness/lua_surface.h>

#include <trx/game/lua/common.h>
#include <trx/game/lua/events.h>

#include <lauxlib.h>

// fake.fire_room_change(item_num, old_room, new_room) - mirrors the
// Item_UpdateRoom fire site, argument for argument.
static int M_L_FireRoomChange(lua_State *const L)
{
    const LUA_EVENT_ARG args[] = {
        { .type = LUA_EVENT_ARG_INT32,
          .value = { .i32 = luaL_checkinteger(L, 1) } },
        { .type = LUA_EVENT_ARG_INT32,
          .value = { .i32 = luaL_checkinteger(L, 2) } },
        { .type = LUA_EVENT_ARG_INT32,
          .value = { .i32 = luaL_checkinteger(L, 3) } },
    };
    LUA_FireEventEx(LUA_EVENT_ROOM_CHANGE, args, 3);
    return 0;
}

// fake.load_next_level() - the rooms are replaced, as they are at a level
// change.
static int M_L_LoadNextLevel(lua_State *const L)
{
    FakeRooms_LoadNextLevel();
    return 0;
}

// Set the world-loaded state because level scripts declare flip groups before
// the level is read.
static int M_L_SetWorldLoaded(lua_State *const L)
{
    FakeLevel_SetWorldLoaded(lua_toboolean(L, 1));
    return 0;
}

// Apply declared flip groups once Level_Initialise has created the rooms.
static int M_L_ApplyFlipGroups(lua_State *const L)
{
    LUA_Rooms_ApplyFlipGroups();
    return 0;
}

// Set whether the current script is a level script, since only level scripts
// may declare flip groups.
static int M_L_SetLevelScript(lua_State *const L)
{
    LUA_SetScriptContext(
        lua_toboolean(L, 1) ? LUA_CONTEXT_LEVEL : LUA_CONTEXT_GLOBAL);
    return 0;
}

static int M_L_ClearFlipGroups(lua_State *const L)
{
    LUA_Rooms_ClearFlipGroups();
    return 0;
}

static void M_PushFake(lua_State *const L)
{
    lua_pushinteger(L, FAKE_ROOM_COUNT);
    lua_setfield(L, -2, "ROOM_COUNT");
    lua_pushcfunction(L, M_L_LoadNextLevel);
    lua_setfield(L, -2, "load_next_level");
    lua_pushcfunction(L, M_L_FireRoomChange);
    lua_setfield(L, -2, "fire_room_change");
    lua_pushcfunction(L, M_L_SetWorldLoaded);
    lua_setfield(L, -2, "set_world_loaded");
    lua_pushcfunction(L, M_L_ApplyFlipGroups);
    lua_setfield(L, -2, "apply_flip_groups");
    lua_pushcfunction(L, M_L_ClearFlipGroups);
    lua_setfield(L, -2, "clear_flip_groups");
    lua_pushcfunction(L, M_L_SetLevelScript);
    lua_setfield(L, -2, "set_level_script");
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "rooms",
        .tests = "api/rooms",
        .deps = { "query", "items", "events" },
        .push_fake = M_PushFake,
    };
    return LuaSurface_Run(&test);
}
