#include <trx/game/objects/general/pickup.h>

#include <lauxlib.h>
#include <lua.h>

void LUA_CreatePickup(lua_State *const L)
{
    lua_getglobal(L, "trxc");
    lua_newtable(L);

    lua_newtable(L);
    lua_pushinteger(L, PICKUP_MODE_NORMAL);
    lua_setfield(L, -2, "NORMAL");
    lua_pushinteger(L, PICKUP_MODE_PLINTH_LOW);
    lua_setfield(L, -2, "PLINTH_LOW");
    lua_pushinteger(L, PICKUP_MODE_PLINTH_HIGH);
    lua_setfield(L, -2, "PLINTH_HIGH");
    lua_setfield(L, -2, "Mode");

    lua_setfield(L, -2, "pickup");
    lua_pop(L, 1);
}
