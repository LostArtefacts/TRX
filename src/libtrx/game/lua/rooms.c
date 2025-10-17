#include "game/lua/common.h"
#include "game/rooms/common.h"

#include <lauxlib.h>

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
    printf("%d\n", idx);
    const ROOM *const room = Room_Get(idx - 1);
    if (room == nullptr) {
        lua_pushnil(L);
    } else {
        lua_pushinteger(L, idx);
    }
    return 1;
}

// trxc.rooms.get_flags(index) → int or nil
static int M_L_RoomGetFlags(lua_State *const L)
{
    const int idx = luaL_checkinteger(L, 1);
    const ROOM *const room = Room_Get(idx - 1);
    if (room == nullptr) {
        lua_pushnil(L);
    } else {
        lua_pushinteger(L, room->flags);
    }
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
    lua_pushcfunction(L, M_L_RoomGetFlags);
    lua_setfield(L, -2, "get_flags");
    lua_setfield(L, -2, "rooms");
    lua_pop(L, 1);
}
