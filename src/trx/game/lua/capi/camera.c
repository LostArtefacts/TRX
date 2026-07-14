#include <trx/game/camera.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>
#include <trx/game/rooms/const.h>

#include <lauxlib.h>

// trxc.camera.get_pos() → {x, y, z}
static int M_L_CameraGetPos(lua_State *const L)
{
    lua_newtable(L);
    lua_pushinteger(L, g_Camera.pos.x);
    lua_setfield(L, -2, "x");
    lua_pushinteger(L, g_Camera.pos.y);
    lua_setfield(L, -2, "y");
    lua_pushinteger(L, g_Camera.pos.z);
    lua_setfield(L, -2, "z");
    return 1;
}

// trxc.camera.get_room() → int (1-based) or nil
static int M_L_CameraGetRoom(lua_State *const L)
{
    if (g_Camera.pos.room_num == NO_ROOM) {
        lua_pushnil(L);
    } else {
        lua_pushinteger(L, g_Camera.pos.room_num + 1);
    }
    return 1;
}

// trxc.camera.get_target_pos() → {x, y, z}
static int M_L_CameraGetTargetPos(lua_State *const L)
{
    lua_newtable(L);
    lua_pushinteger(L, g_Camera.target.x);
    lua_setfield(L, -2, "x");
    lua_pushinteger(L, g_Camera.target.y);
    lua_setfield(L, -2, "y");
    lua_pushinteger(L, g_Camera.target.z);
    lua_setfield(L, -2, "z");
    return 1;
}

// trxc.camera.get_target_room() → int (1-based) or nil
static int M_L_CameraGetTargetRoom(lua_State *const L)
{
    if (g_Camera.target.room_num == NO_ROOM) {
        lua_pushnil(L);
    } else {
        lua_pushinteger(L, g_Camera.target.room_num + 1);
    }
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

static const luaL_Reg m_Module[] = {
    { "get_pos", M_L_CameraGetPos },
    { "get_room", M_L_CameraGetRoom },
    { "get_target_pos", M_L_CameraGetTargetPos },
    { "get_target_room", M_L_CameraGetTargetRoom },
    { "shake", M_L_CameraShake },
    { "reset", M_L_CameraReset },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "camera", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
