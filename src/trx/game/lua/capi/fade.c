#include <trx/core/enum_map.h>
#include <trx/game/lua/events/ui.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils/module.h>
#include <trx/game/output/overlay.h>
#include <trx/game/ui/draw.h>
#include <trx/game/viewport.h>

#include <lauxlib.h>
#include <lua.h>
#include <string.h>

typedef struct {
    BACKGROUND_TYPE style;
    float progress;
    bool has_path;
    char path[];
} M_BACKDROP_ARGS;

static void M_CheckPainting(lua_State *const L)
{
    if (!LUA_UI_IsPainting()) {
        luaL_error(L, "only available while a scene is being painted");
    }
}

static uint8_t M_CheckChannel(lua_State *const L, const int idx)
{
    const lua_Number value = luaL_checknumber(L, idx);
    luaL_argcheck(L, value >= 0.0 && value <= 255.0, idx, "out of range");
    return (uint8_t)(value + 0.5);
}

static float M_CheckOpacity(lua_State *const L, const int idx)
{
    const lua_Number value = luaL_checknumber(L, idx);
    return value < 0.0 ? 0.0f : value > 1.0 ? 1.0f : (float)value;
}

static BACKGROUND_TYPE M_CheckBackdrop(lua_State *const L, const int idx)
{
    const int32_t style =
        ENUM_MAP_GET(BACKGROUND_TYPE, luaL_checkstring(L, idx), -1);
    luaL_argcheck(L, style != -1, idx, "unknown backdrop");
    return (BACKGROUND_TYPE)style;
}

static void M_DrawBackdropOp(const void *const args)
{
    const M_BACKDROP_ARGS *const backdrop = args;
    Output_Overlay_DrawBackground(
        backdrop->style, backdrop->progress,
        backdrop->has_path ? backdrop->path : nullptr);
}

// trxc.fade.capture_snapshot(): integer?
//
// Keeps the frame on screen now in a slot, for a layer that holds it, and
// returns the slot, or nil where every slot is taken.
static int M_L_FadeCaptureSnapshot(lua_State *const L)
{
    const int32_t slot = Output_Overlay_CaptureSnapshotSlot();
    if (slot < 0) {
        lua_pushnil(L);
    } else {
        lua_pushinteger(L, slot);
    }
    return 1;
}

// trxc.fade.free_snapshot(slot: integer)
static int M_L_FadeFreeSnapshot(lua_State *const L)
{
    Output_Overlay_FreeSnapshotSlot((int32_t)luaL_checkinteger(L, 1));
    return 0;
}

// trxc.fade.capture_game()
//
// Keeps the game as it is now, for a backdrop to show behind it.
static int M_L_FadeCaptureGame(lua_State *const L)
{
    Output_Overlay_CaptureGameSnapshot();
    return 0;
}

// trxc.fade.fill(r: number, g: number, b: number, opacity: number)
static int M_L_FadeFill(lua_State *const L)
{
    M_CheckPainting(L);
    const float opacity = M_CheckOpacity(L, 4);
    if (opacity <= 0.0f) {
        return 0;
    }
    const RGBA_8888 color = {
        .r = M_CheckChannel(L, 1),
        .g = M_CheckChannel(L, 2),
        .b = M_CheckChannel(L, 3),
        .a = (uint8_t)(0xFF * opacity + 0.5f),
    };
    UI_ScheduleDrawScreenFlatQuad(
        0, 0, 0, Viewport_GetWidth(VIEWPORT_UI),
        Viewport_GetHeight(VIEWPORT_UI), color);
    return 0;
}

// trxc.fade.snapshot(slot: integer, opacity: number)
static int M_L_FadeSnapshot(lua_State *const L)
{
    M_CheckPainting(L);
    const int32_t slot = (int32_t)luaL_checkinteger(L, 1);
    const float opacity = M_CheckOpacity(L, 2);
    if (opacity > 0.0f) {
        UI_ScheduleDrawSnapshotSlot(slot, opacity);
    }
    return 0;
}

// trxc.fade.backdrop(style: string, opacity: number, path?: string)
//
// A picture comes in from black, so the overlay takes how dark it is; every
// other style takes how far it has come in.
static int M_L_FadeBackdrop(lua_State *const L)
{
    M_CheckPainting(L);
    const BACKGROUND_TYPE style = M_CheckBackdrop(L, 1);
    const float opacity = M_CheckOpacity(L, 2);
    const char *const path = luaL_optstring(L, 3, nullptr);
    const char *const text = path != nullptr ? path : "";
    const size_t path_size = strlen(text) + 1;
    const size_t size = sizeof(M_BACKDROP_ARGS) + path_size;
    M_BACKDROP_ARGS *const args = lua_newuserdatauv(L, size, 0);
    args->style = style;
    args->progress = style == BK_IMAGE ? 1.0f - opacity : opacity;
    args->has_path = path != nullptr;
    memcpy(args->path, text, path_size);
    UI_ScheduleDrawOverlay(M_DrawBackdropOp, args, size);
    lua_pop(L, 1);
    return 0;
}

// trxc.fade.is_backdrop(style: string): boolean
static int M_L_FadeIsBackdrop(lua_State *const L)
{
    lua_pushboolean(
        L, ENUM_MAP_GET(BACKGROUND_TYPE, luaL_checkstring(L, 1), -1) != -1);
    return 1;
}

// trxc.fade.hold_screen(held: boolean)
static int M_L_FadeHoldScreen(lua_State *const L)
{
    luaL_checktype(L, 1, LUA_TBOOLEAN);
    LUA_UI_SetHoldingScreen(lua_toboolean(L, 1));
    return 0;
}

static const luaL_Reg m_Module[] = {
    { "backdrop", M_L_FadeBackdrop },
    { "capture_game", M_L_FadeCaptureGame },
    { "capture_snapshot", M_L_FadeCaptureSnapshot },
    { "fill", M_L_FadeFill },
    { "free_snapshot", M_L_FadeFreeSnapshot },
    { "hold_screen", M_L_FadeHoldScreen },
    { "is_backdrop", M_L_FadeIsBackdrop },
    { "snapshot", M_L_FadeSnapshot },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "fade", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
