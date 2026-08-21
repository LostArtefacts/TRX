#include <trx/game/lua/common.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>
#include <trx/game/random.h>

#include <lauxlib.h>

// trxc.random.next_control() -> integer in [0, RANDOM_SPAN - 1]
static int M_L_NextControl(lua_State *const L)
{
    lua_pushinteger(L, Random_GetControl());
    return 1;
}

// trxc.random.next_draw() -> integer in [0, RANDOM_SPAN - 1]
static int M_L_NextDraw(lua_State *const L)
{
    lua_pushinteger(L, Random_GetDraw());
    return 1;
}

static const luaL_Reg m_Module[] = {
    { "next_control", M_L_NextControl },
    { "next_draw", M_L_NextDraw },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "random", m_Module);

    // The width of a draw belongs to the generator, so Lua reads it rather
    // than repeating it.
    LUA_GetModule(L, "random");
    lua_pushinteger(L, RANDOM_SPAN);
    lua_setfield(L, -2, "SPAN");
    lua_pop(L, 1);
}

REGISTER_LUA_CAPI(.create = M_Create)
