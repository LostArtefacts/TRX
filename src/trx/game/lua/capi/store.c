#include <trx/game/lua/registry.h>
#include <trx/game/lua/store.h>
#include <trx/game/lua/utils.h>

static void M_Create(lua_State *const L)
{
    static const luaL_Reg empty[] = { { nullptr, nullptr } };
    LUA_RegisterModule(L, "store", empty);
    LUA_GetModule(L, "store");

    // trxc.store.level: table
    LUA_Store_PushLevelTable(L);
    lua_setfield(L, -2, "level");

    // trxc.store.game: table
    LUA_Store_PushGameTable(L);
    lua_setfield(L, -2, "game");

    lua_pop(L, 1);
}

REGISTER_LUA_CAPI(.create = M_Create)
