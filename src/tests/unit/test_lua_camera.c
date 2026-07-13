// The camera surface. The assertions live in src/tests/unit/lua/camera.lua;
// this stands up the world they run against.
//
// rooms is loaded alongside it: trx.camera.room hands back a trx.rooms.Room, so
// the declaration reaches across into another module.

#include "fake_engine_camera.h"
#include "fake_engine_rooms.h"
#include "lua_surface.h"

#include <trx/game/camera/types.h>
#include <trx/game/camera/vars.h>

extern void LUA_CreateCamera(lua_State *L);
extern void LUA_CreateRooms(lua_State *L);

static void M_SetUpTRXC(lua_State *const L)
{
    LUA_CreateCamera(L);
    LUA_CreateRooms(L);
}

static int M_FakeReset(lua_State *const L)
{
    FakeCamera_Reset();
    FakeRooms_Reset();
    return 0;
}

static int M_FakeNoRoom(lua_State *const L)
{
    FakeCamera_SetNoRoom();
    return 0;
}

static int M_FakeCalls(lua_State *const L)
{
    lua_newtable(L);
    lua_pushinteger(L, g_FakeCameraCalls.reset);
    lua_setfield(L, -2, "reset");
    lua_pushinteger(L, g_Camera.bounce);
    lua_setfield(L, -2, "bounce");
    return 1;
}

static void M_PushFake(lua_State *const L)
{
    lua_pushinteger(L, FAKE_CAMERA_ROOM);
    lua_setfield(L, -2, "CAMERA_ROOM");
    lua_pushinteger(L, FAKE_CAMERA_TARGET_ROOM);
    lua_setfield(L, -2, "CAMERA_TARGET_ROOM");
    lua_pushcfunction(L, M_FakeNoRoom);
    lua_setfield(L, -2, "set_no_room");
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "camera",
        .deps = { "rooms", nullptr },
        .tests = "camera",
        .setup_trxc = M_SetUpTRXC,
        .push_fake = M_PushFake,
        .fake_reset = M_FakeReset,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
