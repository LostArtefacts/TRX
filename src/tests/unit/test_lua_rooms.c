// The room surface. The assertions live in src/tests/unit/lua/rooms.lua; this
// stands up the world they run against.

#include "fake_engine_items.h"
#include "fake_engine_rooms.h"
#include "lua_surface.h"

#include <trx/game/lua/common.h>
#include <trx/game/lua/events.h>

#include <lauxlib.h>

// The room hooks go through the real listener registry, which asks for the
// script context to scope a listener.
LUA_CONTEXT LUA_GetScriptContext(void)
{
    return LUA_CONTEXT_GLOBAL;
}

static int M_FakeReset(lua_State *const L)
{
    FakeRooms_Reset();
    FakeItems_Reset();
    return 0;
}

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

static int M_FakeCalls(lua_State *const L)
{
    lua_newtable(L);
    lua_pushinteger(L, g_FakeRoomCalls.flip_map);
    lua_setfield(L, -2, "flip_map");
    lua_pushinteger(L, g_FakeRoomCalls.flip_effect);
    lua_setfield(L, -2, "flip_effect");
    lua_pushinteger(L, g_FakeRoomCalls.flip_timer);
    lua_setfield(L, -2, "flip_timer");
    lua_pushboolean(L, g_FakeRoomCalls.fix_tilts);
    lua_setfield(L, -2, "fix_tilts");
    return 1;
}

// fake.load_next_level() - the rooms are replaced, as they are at a level
// change.
static int M_L_LoadNextLevel(lua_State *const L)
{
    FakeRooms_LoadNextLevel();
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
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "rooms",
        .tests = "rooms",
        .deps = { "query", "items", "events" },
        .push_fake = M_PushFake,
        .fake_reset = M_FakeReset,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
