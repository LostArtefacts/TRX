#include <trx/game/creature.h>
#include <trx/game/lua/common.h>

#include <lauxlib.h>

// trxc.creatures.are_allies_hostile() → bool
static int M_L_CreaturesAreAlliesHostile(lua_State *const L)
{
    const bool hostile = Creature_AreAlliesHostile();
    lua_pushboolean(L, hostile);
    return 1;
}

// trxc.creatures.set_allies_hostile(enable)
static int M_L_CreaturesSetAlliesHostile(lua_State *const L)
{
    const bool hostile = lua_toboolean(L, 1) != 0;
    Creature_SetAlliesHostile(hostile);
    return 0;
}

void LUA_CreateCreatures(lua_State *const L)
{
    lua_getglobal(L, "trxc");
    lua_newtable(L);
    lua_pushcfunction(L, M_L_CreaturesAreAlliesHostile);
    lua_setfield(L, -2, "are_allies_hostile");
    lua_pushcfunction(L, M_L_CreaturesSetAlliesHostile);
    lua_setfield(L, -2, "set_allies_hostile");
    lua_setfield(L, -2, "creatures");
    lua_pop(L, 1);
}
