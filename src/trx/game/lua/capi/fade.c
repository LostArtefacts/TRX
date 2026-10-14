#include <trx/core/enum_map.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils/module.h>
#include <trx/game/ui/fade.h>

#include <lauxlib.h>
#include <lua.h>

static uint8_t M_CheckChannel(lua_State *const L, const int idx)
{
    const lua_Number value = luaL_checknumber(L, idx);
    luaL_argcheck(L, value >= 0.0 && value <= 255.0, idx, "out of range");
    return (uint8_t)(value + 0.5);
}

// trxc.fade.push_fill(r: number, g: number, b: number, opacity: number): integer
static int M_L_FadePushFill(lua_State *const L)
{
    const RGBA_8888 color = {
        .r = M_CheckChannel(L, 1),
        .g = M_CheckChannel(L, 2),
        .b = M_CheckChannel(L, 3),
        .a = 0xFF,
    };
    lua_pushinteger(L, UI_Fade_PushFill(color, (float)luaL_checknumber(L, 4)));
    return 1;
}

// trxc.fade.push_snapshot(opacity: number): integer
static int M_L_FadePushSnapshot(lua_State *const L)
{
    lua_pushinteger(L, UI_Fade_PushSnapshot((float)luaL_checknumber(L, 1)));
    return 1;
}

// trxc.fade.push_backdrop(style: string, path: string?, opacity: number): integer
static int M_L_FadePushBackdrop(lua_State *const L)
{
    const int32_t style =
        ENUM_MAP_GET(BACKGROUND_TYPE, luaL_checkstring(L, 1), -1);
    luaL_argcheck(L, style != -1, 1, "unknown backdrop");
    lua_pushinteger(
        L,
        UI_Fade_PushBackdrop(
            (BACKGROUND_TYPE)style, luaL_optstring(L, 2, nullptr),
            (float)luaL_checknumber(L, 3)));
    return 1;
}

// trxc.fade.to(id: integer, target: number, ticks: integer, curve: string, close: boolean)
static int M_L_FadeTo(lua_State *const L)
{
    const char *const curve = luaL_checkstring(L, 4);
    const int32_t ease = ENUM_MAP_GET(UI_EASE, curve, -1);
    luaL_argcheck(L, ease != -1, 4, "unknown curve");
    UI_Fade_To(
        (int32_t)luaL_checkinteger(L, 1), (float)luaL_checknumber(L, 2),
        (int32_t)luaL_checkinteger(L, 3), (UI_EASE)ease, lua_toboolean(L, 5));
    return 0;
}

// trxc.fade.is_open(id: integer): boolean
static int M_L_FadeIsOpen(lua_State *const L)
{
    lua_pushboolean(L, UI_Fade_IsOpen((int32_t)luaL_checkinteger(L, 1)));
    return 1;
}

// trxc.fade.opacity(id: integer): number
static int M_L_FadeOpacity(lua_State *const L)
{
    lua_pushnumber(L, UI_Fade_GetOpacity((int32_t)luaL_checkinteger(L, 1)));
    return 1;
}

// trxc.fade.hold_screen(id: integer)
static int M_L_FadeHoldScreen(lua_State *const L)
{
    UI_Fade_HoldScreen((int32_t)luaL_checkinteger(L, 1));
    return 0;
}

// trxc.fade.close(id: integer)
static int M_L_FadeClose(lua_State *const L)
{
    UI_Fade_Close((int32_t)luaL_checkinteger(L, 1));
    return 0;
}

static const luaL_Reg m_Module[] = {
    { "close", M_L_FadeClose },
    { "hold_screen", M_L_FadeHoldScreen },
    { "is_open", M_L_FadeIsOpen },
    { "opacity", M_L_FadeOpacity },
    { "push_backdrop", M_L_FadePushBackdrop },
    { "push_fill", M_L_FadePushFill },
    { "push_snapshot", M_L_FadePushSnapshot },
    { "to", M_L_FadeTo },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "fade", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
