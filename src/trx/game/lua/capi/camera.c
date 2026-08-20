#include <trx/game/camera.h>
#include <trx/game/flyby_mode.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>
#include <trx/game/rooms/const.h>

#include <lauxlib.h>

// trxc.camera.get_pos() → {x, y, z}
static int M_L_CameraGetPos(lua_State *const L)
{
    LUA_PushXYZ(L, g_Camera.pos.pos);
    return 1;
}

// trxc.camera.get_room() → int (0-based) or nil
static int M_L_CameraGetRoom(lua_State *const L)
{
    LUA_PushOptIndex(L, g_Camera.pos.room_num, NO_ROOM);
    return 1;
}

// trxc.camera.get_target_pos() → {x, y, z}
static int M_L_CameraGetTargetPos(lua_State *const L)
{
    LUA_PushXYZ(L, g_Camera.target.pos);
    return 1;
}

// trxc.camera.get_target_room() → int (0-based) or nil
static int M_L_CameraGetTargetRoom(lua_State *const L)
{
    LUA_PushOptIndex(L, g_Camera.target.room_num, NO_ROOM);
    return 1;
}

// trxc.camera.shake(intensity)
static int M_L_CameraShake(lua_State *const L)
{
    g_Camera.bounce = (int32_t)luaL_checkinteger(L, 1);
    return 0;
}

// trxc.camera.reset()
static int M_L_CameraReset(lua_State *const L)
{
    Camera_ResetPosition();
    return 0;
}

// trxc.camera.is_flyby_active() -> bool
static int M_L_CameraIsFlybyActive(lua_State *const L)
{
    lua_pushboolean(L, FlybyMode_IsActive());
    return 1;
}

// trxc.camera.play_flyby(sequence_num)
static int M_L_CameraPlayFlyby(lua_State *const L)
{
    lua_pushboolean(
        L, FlybyMode_Activate((int32_t)luaL_checkinteger(L, 1), false));
    return 1;
}

// trxc.camera.cancel_flyby()
static int M_L_CameraCancelFlyby(lua_State *const L)
{
    FlybyMode_Cancel(true);
    return 0;
}

static const luaL_Reg m_Module[] = {
    { "get_pos", M_L_CameraGetPos },
    { "get_room", M_L_CameraGetRoom },
    { "get_target_pos", M_L_CameraGetTargetPos },
    { "get_target_room", M_L_CameraGetTargetRoom },
    { "shake", M_L_CameraShake },
    { "reset", M_L_CameraReset },
    { "is_flyby_active", M_L_CameraIsFlybyActive },
    { "play_flyby", M_L_CameraPlayFlyby },
    { "cancel_flyby", M_L_CameraCancelFlyby },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "camera", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
