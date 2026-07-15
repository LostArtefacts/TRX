#include <trx/game/creature.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>

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
    const bool hostile = lua_toboolean(L, 1);
    Creature_SetAlliesHostile(hostile);
    return 0;
}

// trxc.creatures.add_ally(obj_id)
static int M_L_CreaturesAddAlly(lua_State *const L)
{
    Creature_AddAlly(LUA_CheckObjectID(L, 1));
    return 0;
}

// trxc.creatures.add_ally_target(obj_id)
static int M_L_CreaturesAddAllyTarget(lua_State *const L)
{
    Creature_AddAllyTargetingEnemy(LUA_CheckObjectID(L, 1));
    return 0;
}

static const luaL_Reg m_Module[] = {
    { "are_allies_hostile", M_L_CreaturesAreAlliesHostile },
    { "set_allies_hostile", M_L_CreaturesSetAlliesHostile },
    { "add_ally", M_L_CreaturesAddAlly },
    { "add_ally_target", M_L_CreaturesAddAllyTarget },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "creatures", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
