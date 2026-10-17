#include <trx/game/lua/common.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils/args.h>
#include <trx/game/lua/utils/module.h>
#include <trx/game/output/overlay.h>
#include <trx/game/ui/menu.h>

#include <lauxlib.h>

// trxc.overlay.has_letterbox(): boolean
static int M_L_OverlayHasLetterbox(lua_State *const L)
{
    lua_pushboolean(L, Output_Overlay_HasLetterbox());
    return 1;
}

// trxc.overlay.get_letterbox(): number
static int M_L_OverlayGetLetterbox(lua_State *const L)
{
    lua_pushnumber(L, Output_Overlay_GetLetterboxTarget());
    return 1;
}

// trxc.overlay.set_letterbox(depth: number)
static int M_L_OverlaySetLetterbox(lua_State *const L)
{
    const float depth = (float)luaL_checknumber(L, 1);
    luaL_argcheck(
        L, depth >= 0.0f && depth <= 0.5f, 1, "depth must be from 0 to 0.5");
    Output_Overlay_SlideLetterbox(depth);
    return 0;
}

// trxc.overlay.show_pickup(object: trx.catalog.objects)
static int M_L_OverlayShowPickup(lua_State *const L)
{
    UI_ShowPickup(LUA_CheckObjectID(L, 1));
    return 0;
}

static const luaL_Reg m_Module[] = {
    { "get_letterbox", M_L_OverlayGetLetterbox },
    { "has_letterbox", M_L_OverlayHasLetterbox },
    { "set_letterbox", M_L_OverlaySetLetterbox },
    { "show_pickup", M_L_OverlayShowPickup },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "overlay", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
