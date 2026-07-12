// The room surface. The assertions live in src/tests/unit/lua/rooms.lua; this
// stands up the world they run against.

#include "fake_engine_rooms.h"
#include "lua_surface.h"

extern void LUA_CreateRooms(lua_State *L);

static void M_SetUpTRXC(lua_State *const L)
{
    LUA_CreateRooms(L);
}

static int M_FakeReset(lua_State *const L)
{
    FakeRooms_Reset();
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
    return 1;
}

static void M_PushFake(lua_State *const L)
{
    lua_pushinteger(L, FAKE_ROOM_COUNT);
    lua_setfield(L, -2, "ROOM_COUNT");
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "rooms",
        .tests = "rooms",
        .setup_trxc = M_SetUpTRXC,
        .push_fake = M_PushFake,
        .fake_reset = M_FakeReset,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
