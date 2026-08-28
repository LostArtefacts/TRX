#include <trx/game/lua/common.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>
#include <trx/game/output/overlay.h>
#include <trx/game/overlay.h>

#include <lauxlib.h>

// trxc.overlay.is_health_bar_forced() -> bool
static int M_L_OverlayIsHealthBarForced(lua_State *const L)
{
    lua_pushboolean(L, Overlay_IsHealthBarForced());
    return 1;
}

// trxc.overlay.has_letterbox() -> bool
static int M_L_OverlayHasLetterbox(lua_State *const L)
{
    lua_pushboolean(L, Output_Overlay_HasLetterbox());
    return 1;
}

static const luaL_Reg m_Module[] = {
    { "has_letterbox", M_L_OverlayHasLetterbox },
    { "is_health_bar_forced", M_L_OverlayIsHealthBarForced },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "overlay", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
