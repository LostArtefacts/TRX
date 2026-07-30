// The camera surface. The assertions live in camera.lua;
// this stands up the world they run against.
//
// rooms is loaded alongside it: trx.camera.room hands back a trx.rooms.Room, so
// the declaration reaches across into another module.

#include <fakes/camera.h>
#include <fakes/rooms.h>
#include <harness/lua_surface.h>

#include <trx/game/camera/types.h>
#include <trx/game/camera/vars.h>

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

static int M_FakeStartFlyby(lua_State *const L)
{
    FakeCamera_SetFlybyActive(true);
    return 0;
}

static int M_FakeCalls(lua_State *const L)
{
    lua_newtable(L);
    lua_pushinteger(L, g_FakeCameraCalls.reset);
    lua_setfield(L, -2, "reset");
    lua_pushinteger(L, g_Camera.bounce);
    lua_setfield(L, -2, "bounce");
    lua_pushinteger(L, g_FakeCameraCalls.cancel_flyby);
    lua_setfield(L, -2, "cancel_flyby");
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
    lua_pushcfunction(L, M_FakeStartFlyby);
    lua_setfield(L, -2, "start_flyby");
}

int main(void)
{
    const LUA_SURFACE_TEST test = {
        .module = "camera",
        .deps = { "rooms", nullptr },
        .tests = "api/camera",
        .push_fake = M_PushFake,
        .fake_reset = M_FakeReset,
        .fake_calls = M_FakeCalls,
    };
    return LuaSurface_Run(&test);
}
