#include <trx/core/memory.h>
#include <trx/game/game_strings/entries.h>
#include <trx/game/inventory_ring/control.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils/args.h>
#include <trx/game/lua/utils/module.h>
#include <trx/game/output/overlay.h>
#include <trx/game/overlay.h>

#include <lauxlib.h>

// Keep the caption until the next caption replaces it.
static char *m_CaptionText = nullptr;

// trxc.overlay.is_health_bar_forced(): boolean
static int M_L_OverlayIsHealthBarForced(lua_State *const L)
{
    lua_pushboolean(L, Overlay_IsHealthBarForced());
    return 1;
}

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
    Overlay_AddDisplayPickup(LUA_CheckObjectID(L, 1));
    return 0;
}

// trxc.overlay.set_caption(text?: string, count?: integer)
//
// Set the interface caption, and the count that stands with it. Nil removes
// them.
static int M_L_OverlaySetCaption(lua_State *const L)
{
    if (lua_isnoneornil(L, 2)) {
        InvRing_ClearItemQuantity();
    } else {
        InvRing_ShowItemQuantity("%d", (int32_t)luaL_checkinteger(L, 2));
    }

    if (lua_isnoneornil(L, 1)) {
        Overlay_SetBottomText((OVERLAY_TEXT) {});
        Memory_FreePointer(&m_CaptionText);
        return 0;
    }

    char *const text = Memory_DupStr(luaL_checkstring(L, 1));
    Overlay_SetBottomText((OVERLAY_TEXT) {
        .kind = OVERLAY_TEXT_LITERAL,
        .literal = text,
        .fmt_gs_key = GS_ID("general/inventory_ring/object_name_fmt"),
    });
    // Replace the old text after the new text is validated.
    Memory_FreePointer(&m_CaptionText);
    m_CaptionText = text;
    return 0;
}

// trxc.overlay.show_arrow(arrow: trx.overlay.Arrow, shown?: boolean)
static int M_L_OverlayShowArrow(lua_State *const L)
{
    const OVERLAY_ARROW arrow = (OVERLAY_ARROW)LUA_CheckRange(
        L, 1, OVERLAY_ARROW_NUMBER_OF, "unknown arrow");
    Overlay_ShowArrow(arrow, lua_toboolean(L, 2));
    return 0;
}

static const luaL_Reg m_Module[] = {
    { "get_letterbox", M_L_OverlayGetLetterbox },
    { "has_letterbox", M_L_OverlayHasLetterbox },
    { "set_letterbox", M_L_OverlaySetLetterbox },
    { "set_caption", M_L_OverlaySetCaption },
    { "show_arrow", M_L_OverlayShowArrow },
    { "is_health_bar_forced", M_L_OverlayIsHealthBarForced },
    { "show_pickup", M_L_OverlayShowPickup },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "overlay", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
