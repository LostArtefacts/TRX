#include "game/lua/common.h"
#include "game/rooms.h"

#include <lauxlib.h>

#define M_ROOM_GETTER(L)                                                       \
    const int idx = luaL_checkinteger(L, 1);                                   \
    const ROOM *const room = Room_Get(idx - 1);                                \
    if (room == nullptr) {                                                     \
        lua_pushnil(L);                                                        \
        return 1;                                                              \
    }

#define M_ROOM_SETTER(L)                                                       \
    const int idx = luaL_checkinteger(L, 1);                                   \
    ROOM *const room = Room_Get(idx - 1);                                      \
    if (room == nullptr) {                                                     \
        return 1;                                                              \
    }

// trxc.rooms.count() → int
static int M_L_RoomsCount(lua_State *const L)
{
    lua_pushinteger(L, Room_GetCount());
    return 1;
}

// trxc.rooms.get(index) → int (1-based) or nil
static int M_L_RoomsGet(lua_State *const L)
{
    const int idx = luaL_checkinteger(L, 1);
    const ROOM *const room = Room_Get(idx - 1);
    if (room == nullptr) {
        lua_pushnil(L);
    } else {
        lua_pushinteger(L, idx);
    }
    return 1;
}

// trxc.rooms.get_underwater(index) → bool or nil
static int M_L_RoomGetUnderwater(lua_State *const L)
{
    M_ROOM_GETTER(L);
    lua_pushboolean(L, room->flags.underwater);
    return 1;
}

// trxc.rooms.get_wind(index) → bool or nil
static int M_L_RoomGetWind(lua_State *const L)
{
    M_ROOM_GETTER(L);
    lua_pushboolean(L, room->flags.wind);
    return 1;
}

// trxc.rooms.set_underwater(index, bool)
static int M_L_RoomSetUnderwater(lua_State *const L)
{
    M_ROOM_SETTER(L);
    room->flags.underwater = lua_toboolean(L, 2);
    return 1;
}

// trxc.rooms.set_wind(index, bool)
static int M_L_RoomSetWind(lua_State *const L)
{
    M_ROOM_SETTER(L);
    room->flags.wind = lua_toboolean(L, 2);
    return 1;
}

// trxc.rooms.get_bounds() → table
static int M_L_RoomsBounds(lua_State *const L)
{
    M_ROOM_GETTER(L);
    const BOUNDS_32 bounds = Room_GetRoomBounds(idx - 1);
    lua_newtable(L);
    lua_pushinteger(L, bounds.min.x);
    lua_setfield(L, -2, "min_x");
    lua_pushinteger(L, bounds.min.y);
    lua_setfield(L, -2, "min_y");
    lua_pushinteger(L, bounds.min.z);
    lua_setfield(L, -2, "min_z");
    lua_pushinteger(L, bounds.max.x);
    lua_setfield(L, -2, "max_x");
    lua_pushinteger(L, bounds.max.y);
    lua_setfield(L, -2, "max_y");
    lua_pushinteger(L, bounds.max.z);
    lua_setfield(L, -2, "max_z");
    return 1;
}

void LUA_CreateRooms(lua_State *const L)
{
    lua_getglobal(L, "trxc");
    lua_newtable(L);
    lua_pushcfunction(L, M_L_RoomsCount);
    lua_setfield(L, -2, "count");
    lua_pushcfunction(L, M_L_RoomsGet);
    lua_setfield(L, -2, "get");
    lua_pushcfunction(L, M_L_RoomGetUnderwater);
    lua_setfield(L, -2, "get_underwater");
    lua_pushcfunction(L, M_L_RoomGetWind);
    lua_setfield(L, -2, "get_wind");
    lua_pushcfunction(L, M_L_RoomSetUnderwater);
    lua_setfield(L, -2, "set_underwater");
    lua_pushcfunction(L, M_L_RoomSetWind);
    lua_setfield(L, -2, "set_wind");
    lua_pushcfunction(L, M_L_RoomsBounds);
    lua_setfield(L, -2, "get_bounds");
    lua_setfield(L, -2, "rooms");
    lua_pop(L, 1);
}
