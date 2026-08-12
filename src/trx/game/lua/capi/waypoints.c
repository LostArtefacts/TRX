#include <trx/game/lua/common.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>
#include <trx/game/waypoint.h>

#include <lauxlib.h>

// trxc.waypoints.get_current() → int or nil
static int M_L_WaypointsGetCurrent(lua_State *const L)
{
    LUA_PushOptIndex(L, Waypoint_Get(), WAYPOINT_NONE);
    return 1;
}

// trxc.waypoints.set_current(num)
static int M_L_WaypointsSetCurrent(lua_State *const L)
{
    Waypoint_Set((int32_t)luaL_checkinteger(L, 1));
    return 0;
}

// trxc.waypoints.get_highest() → int or nil
static int M_L_WaypointsGetHighest(lua_State *const L)
{
    LUA_PushOptIndex(L, Waypoint_GetHighest(), WAYPOINT_NONE);
    return 1;
}

// trxc.waypoints.get_pad() → int or nil
static int M_L_WaypointsGetPad(lua_State *const L)
{
    LUA_PushOptIndex(L, Waypoint_GetPad(), WAYPOINT_PAD_NONE);
    return 1;
}

// trxc.waypoints.set_pad(num or nil)
static int M_L_WaypointsSetPad(lua_State *const L)
{
    if (lua_isnoneornil(L, 1)) {
        Waypoint_ClearPad();
        return 0;
    }
    Waypoint_SetPad((int32_t)luaL_checkinteger(L, 1));
    return 0;
}

static const luaL_Reg m_Module[] = {
    { "get_current", M_L_WaypointsGetCurrent },
    { "set_current", M_L_WaypointsSetCurrent },
    { "get_highest", M_L_WaypointsGetHighest },
    { "get_pad", M_L_WaypointsGetPad },
    { "set_pad", M_L_WaypointsSetPad },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "waypoints", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
