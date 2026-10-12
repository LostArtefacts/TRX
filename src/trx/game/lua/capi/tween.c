#include <trx/core/enum_map.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils/module.h>
#include <trx/game/ui/tween.h>

#include <lauxlib.h>
#include <lua.h>

#define M_METATABLE "trx.ui.tween"

static UI_TWEEN *M_Check(lua_State *const L, const int idx)
{
    return luaL_checkudata(L, idx, M_METATABLE);
}

static UI_EASE M_CheckEase(lua_State *const L, const int idx)
{
    const char *const name = luaL_checkstring(L, idx);
    const int32_t ease = ENUM_MAP_GET(UI_EASE, name, -1);
    luaL_argcheck(L, ease != -1, idx, "unknown curve");
    return (UI_EASE)ease;
}

// trxc.tween.new(value: number): userdata
static int M_L_TweenNew(lua_State *const L)
{
    UI_TWEEN *const tween = lua_newuserdatauv(L, sizeof(UI_TWEEN), 0);
    *tween = (UI_TWEEN) { .ease = UI_EASE_LINEAR };
    UI_Tween_Set(tween, (float)luaL_checknumber(L, 1));
    luaL_setmetatable(L, M_METATABLE);
    return 1;
}

// trxc.tween.get(tween: userdata): number
static int M_L_TweenGet(lua_State *const L)
{
    lua_pushnumber(L, UI_Tween_Get(M_Check(L, 1)));
    return 1;
}

// trxc.tween.target(tween: userdata): number
static int M_L_TweenTarget(lua_State *const L)
{
    lua_pushnumber(L, UI_Tween_GetTarget(M_Check(L, 1)));
    return 1;
}

// trxc.tween.is_moving(tween: userdata): boolean
static int M_L_TweenIsMoving(lua_State *const L)
{
    lua_pushboolean(L, UI_Tween_IsMoving(M_Check(L, 1)));
    return 1;
}

// trxc.tween.to(tween: userdata, target: number, ticks: integer, curve: string, delay: integer)
static int M_L_TweenTo(lua_State *const L)
{
    UI_Tween_To(
        M_Check(L, 1), (float)luaL_checknumber(L, 2),
        (int32_t)luaL_checkinteger(L, 3), (int32_t)luaL_optinteger(L, 5, 0),
        M_CheckEase(L, 4));
    return 0;
}

// trxc.tween.set(tween: userdata, value: number)
static int M_L_TweenSet(lua_State *const L)
{
    UI_Tween_Set(M_Check(L, 1), (float)luaL_checknumber(L, 2));
    return 0;
}

// trxc.tween.ease(curve: string, ratio: number): number
static int M_L_TweenEase(lua_State *const L)
{
    lua_pushnumber(
        L, UI_Ease_Apply(M_CheckEase(L, 1), (float)luaL_checknumber(L, 2)));
    return 1;
}

static const luaL_Reg m_Module[] = {
    { "ease", M_L_TweenEase },
    { "get", M_L_TweenGet },
    { "is_moving", M_L_TweenIsMoving },
    { "new", M_L_TweenNew },
    { "set", M_L_TweenSet },
    { "target", M_L_TweenTarget },
    { "to", M_L_TweenTo },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    luaL_newmetatable(L, M_METATABLE);
    lua_pop(L, 1);
    LUA_RegisterModule(L, "tween", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
