#include "game/lua/common.h"
#include "game/rooms/common.h"

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

// trxc.rooms.set_underwater(index, bool)
static int M_L_RoomSetUnderwater(lua_State *const L)
{
    M_ROOM_SETTER(L);
    room->flags.underwater = lua_toboolean(L, 2);
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
    lua_pushcfunction(L, M_L_RoomSetUnderwater);
    lua_setfield(L, -2, "set_underwater");
    lua_setfield(L, -2, "rooms");
    lua_pop(L, 1);
}
