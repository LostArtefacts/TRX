#include <trx/core/math.h>
#include <trx/game/const.h>
#include <trx/game/lua/common.h>

#include <lauxlib.h>

// Thin wrappers over the engine's fixed-point trig. Scripts could approximate
// these with Lua's math library, but TRX angles are int16 units (65536 = 360
// degrees) and the engine uses lookup tables; going through the real functions
// guarantees a script places things exactly where C would.

// trxc.math.sin(angle) -> number in [-1, 1]
static int M_L_Sin(lua_State *const L)
{
    const int32_t angle = luaL_checkinteger(L, 1);
    lua_pushnumber(L, (double)Math_Sin(angle) / (double)(1 << W2V_SHIFT));
    return 1;
}

// trxc.math.cos(angle) -> number in [-1, 1]
static int M_L_Cos(lua_State *const L)
{
    const int32_t angle = luaL_checkinteger(L, 1);
    lua_pushnumber(L, (double)Math_Cos(angle) / (double)(1 << W2V_SHIFT));
    return 1;
}

// trxc.math.atan(z, x) -> angle
static int M_L_Atan(lua_State *const L)
{
    const int32_t z = luaL_checkinteger(L, 1);
    const int32_t x = luaL_checkinteger(L, 2);
    lua_pushinteger(L, Math_Atan(z, x));
    return 1;
}

void LUA_CreateMath(lua_State *const L)
{
    lua_getglobal(L, "trxc");
    lua_newtable(L);
    lua_pushcfunction(L, M_L_Sin);
    lua_setfield(L, -2, "sin");
    lua_pushcfunction(L, M_L_Cos);
    lua_setfield(L, -2, "cos");
    lua_pushcfunction(L, M_L_Atan);
    lua_setfield(L, -2, "atan");
    lua_pushinteger(L, DEG_1);
    lua_setfield(L, -2, "DEG_1");
    lua_pushinteger(L, DEG_45);
    lua_setfield(L, -2, "DEG_45");
    lua_pushinteger(L, DEG_90);
    lua_setfield(L, -2, "DEG_90");
    lua_pushinteger(L, WALL_L);
    lua_setfield(L, -2, "WALL_L");
    lua_setfield(L, -2, "math");
    lua_pop(L, 1);
}
