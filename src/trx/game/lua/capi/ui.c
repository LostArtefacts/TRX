#include <trx/core/utils.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/ui.h>
#include <trx/game/lua/utils.h>
#include <trx/game/ui/common.h>
#include <trx/game/ui/draw.h>
#include <trx/game/ui/elements.h>
#include <trx/game/ui/regions.h>
#include <trx/game/ui/scaler.h>
#include <trx/game/ui/settings.h>
#include <trx/game/ui/text.h>

#include <lauxlib.h>
#include <math.h>

static bool m_Drawing = false;
static bool m_Painting = false;

static void M_CheckDrawing(lua_State *const L)
{
    if (!m_Drawing) {
        luaL_error(L, "trx.ui is only available from trx.events.on_ui_draw");
    }
}

static float M_OptNumberField(
    lua_State *const L, const int arg, const char *const key,
    const float fallback)
{
    lua_getfield(L, arg, key);
    const float value = (float)luaL_optnumber(L, -1, fallback);
    lua_pop(L, 1);
    return value;
}

static int M_L_UICanvasWidth(lua_State *const L)
{
    lua_pushnumber(L, UI_GetCanvasWidth());
    return 1;
}

static int M_L_UICanvasHeight(lua_State *const L)
{
    lua_pushnumber(L, UI_GetCanvasHeight());
    return 1;
}

static int M_L_UISafeWidth(lua_State *const L)
{
    lua_pushnumber(L, UI_GetSafeCanvasWidth());
    return 1;
}

static int M_L_UISafeTop(lua_State *const L)
{
    lua_pushnumber(L, UI_GetSafeCanvasTop());
    return 1;
}

static int M_L_UISafeBottom(lua_State *const L)
{
    lua_pushnumber(L, UI_GetSafeCanvasBottom());
    return 1;
}

static void M_CheckPainting(lua_State *const L)
{
    if (!LUA_UI_IsPainting()) {
        luaL_error(L, "only available while a scene is being painted");
    }
}

// Accepts any numeric channel and clamps it to uint8_t range.
static uint8_t M_CheckChannel(
    lua_State *const L, const int arg, const char *const key,
    const double fallback)
{
    lua_getfield(L, arg, key);
    double value = luaL_optnumber(L, -1, fallback);
    lua_pop(L, 1);
    CLAMP(value, 0.0, 255.0);
    return (uint8_t)value;
}

// Reads a trx.math.Color table. Alpha defaults to opaque.
static RGBA_8888 M_CheckColor(lua_State *const L, const int arg)
{
    luaL_checktype(L, arg, LUA_TTABLE);
    return (RGBA_8888) {
        .r = M_CheckChannel(L, arg, "r", 0.0),
        .g = M_CheckChannel(L, arg, "g", 0.0),
        .b = M_CheckChannel(L, arg, "b", 0.0),
        .a = M_CheckChannel(L, arg, "a", 255.0),
    };
}

// trxc.ui.reserve(region, w, h) -> slot
static int M_L_UIReserve(lua_State *const L)
{
    M_CheckDrawing(L);
    const lua_Integer region = luaL_checkinteger(L, 1);
    if (region < 0 || region >= UI_REGION_NUMBER_OF) {
        return luaL_error(L, "unknown region");
    }
    lua_pushinteger(
        L,
        UI_Region_Reserve(
            (UI_REGION)region, (float)luaL_checknumber(L, 2),
            (float)luaL_checknumber(L, 3)));
    return 1;
}

// trxc.ui.slot_box(slot) -> x, y, w, h or nil
static int M_L_UISlotBox(lua_State *const L)
{
    float x;
    float y;
    float w;
    float h;
    if (!UI_Region_GetSlotBox(
            (int32_t)luaL_checkinteger(L, 1), &x, &y, &w, &h)) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushnumber(L, x);
    lua_pushnumber(L, y);
    lua_pushnumber(L, w);
    lua_pushnumber(L, h);
    return 4;
}

// trxc.ui.measure_text(text, scale) -> w, h
static int M_L_UIMeasureText(lua_State *const L)
{
    float w;
    float h;
    UI_Text_Measure(
        luaL_checkstring(L, 1), &w, &h,
        (UI_TEXT_SETTINGS) { .scale = (float)luaL_optnumber(L, 2, 1.0) });
    lua_pushnumber(L, w);
    lua_pushnumber(L, h);
    return 2;
}

// trxc.ui.draw_text(text, x, y, scale, z)
static int M_L_UIDrawText(lua_State *const L)
{
    M_CheckPainting(L);
    UI_Text_Draw(
        luaL_checkstring(L, 1), (float)luaL_checknumber(L, 2),
        (float)luaL_checknumber(L, 3),
        (UI_TEXT_SETTINGS) {
            .scale = (float)luaL_optnumber(L, 4, 1.0),
            .z = (int32_t)luaL_optinteger(L, 5, 0),
        });
    return 0;
}

// Convert both rectangle edges so adjacent canvas edges round together.

// trxc.ui.flat_quad(x, y, z, w, h, color)
static int M_L_UIFlatQuad(lua_State *const L)
{
    M_CheckPainting(L);
    const float x = (float)luaL_checknumber(L, 1);
    const float y = (float)luaL_checknumber(L, 2);
    const int32_t z = (int32_t)luaL_optinteger(L, 3, 0);
    const int32_t x0 = lroundf(UI_ScaleX(x));
    const int32_t y0 = lroundf(UI_ScaleY(y));
    const int32_t w =
        (int32_t)lroundf(UI_ScaleX(x + (float)luaL_checknumber(L, 4))) - x0;
    const int32_t h =
        (int32_t)lroundf(UI_ScaleY(y + (float)luaL_checknumber(L, 5))) - y0;
    UI_ScheduleDrawScreenFlatQuad(x0, y0, z, w, h, M_CheckColor(L, 6));
    return 0;
}

// trxc.ui.gradient_quad(x, y, z, w, h, tl, tr, bl, br)
static int M_L_UIGradientQuad(lua_State *const L)
{
    M_CheckPainting(L);
    const float x = (float)luaL_checknumber(L, 1);
    const float y = (float)luaL_checknumber(L, 2);
    const int32_t z = (int32_t)luaL_optinteger(L, 3, 0);
    const int32_t x0 = lroundf(UI_ScaleX(x));
    const int32_t y0 = lroundf(UI_ScaleY(y));
    const int32_t w =
        (int32_t)lroundf(UI_ScaleX(x + (float)luaL_checknumber(L, 4))) - x0;
    const int32_t h =
        (int32_t)lroundf(UI_ScaleY(y + (float)luaL_checknumber(L, 5))) - y0;
    UI_ScheduleDrawScreenGradientQuad(
        x0, y0, z, w, h, M_CheckColor(L, 6), M_CheckColor(L, 7),
        M_CheckColor(L, 8), M_CheckColor(L, 9));
    return 0;
}

// trxc.ui.to_screen(canvas) -> number
static int M_L_UIToScreen(lua_State *const L)
{
    lua_pushnumber(L, UI_ScaleY((float)luaL_checknumber(L, 1)));
    return 1;
}

// trxc.ui.to_canvas(screen) -> number
static int M_L_UIToCanvas(lua_State *const L)
{
    const float per_unit = UI_ScaleY(1.0f);
    lua_pushnumber(
        L, per_unit != 0.0f ? (float)luaL_checknumber(L, 1) / per_unit : 0.0f);
    return 1;
}

// Pushes a trx.math.Color table.
static void M_PushColor(lua_State *const L, const RGBA_8888 color)
{
    lua_createtable(L, 0, 4);
    lua_pushinteger(L, color.r);
    lua_setfield(L, -2, "r");
    lua_pushinteger(L, color.g);
    lua_setfield(L, -2, "g");
    lua_pushinteger(L, color.b);
    lua_setfield(L, -2, "b");
    lua_pushinteger(L, color.a);
    lua_setfield(L, -2, "a");
}

static void M_PushRamp(
    lua_State *const L, const RGBA_8888 *const ramp, const char *const name)
{
    lua_createtable(L, UI_BAR_COLOR_STEPS, 0);
    for (int32_t i = 0; i < UI_BAR_COLOR_STEPS; i++) {
        M_PushColor(L, ramp[i]);
        lua_rawseti(L, -2, i + 1);
    }
    lua_setfield(L, -2, name);
}

// trxc.ui.bar_theme(type) -> table or nil
//
// Returns the current bar theme from player settings.
static int M_L_UIBarTheme(lua_State *const L)
{
    const lua_Integer type = luaL_checkinteger(L, 1);
    if (type < 0 || type >= UI_BAR_NUMBER_OF) {
        return luaL_error(L, "unknown bar type");
    }
    const UI_BAR_THEME *const theme =
        UI_Settings_GetBarTheme((UI_BAR_TYPE)type);
    if (theme == nullptr) {
        lua_pushnil(L);
        return 1;
    }

    lua_createtable(L, 0, 9);
    lua_pushstring(L, theme->kind == UI_BAR_THEME_PS1_KIND ? "ps1" : "pc");
    lua_setfield(L, -2, "kind");
    lua_pushnumber(L, theme->basic_scale);
    lua_setfield(L, -2, "basic_scale");
    M_PushColor(L, theme->border_light);
    lua_setfield(L, -2, "border_light");
    M_PushColor(L, theme->border_dark);
    lua_setfield(L, -2, "border_dark");
    M_PushColor(L, theme->border_tl);
    lua_setfield(L, -2, "border_tl");
    M_PushColor(L, theme->border_tr);
    lua_setfield(L, -2, "border_tr");
    M_PushColor(L, theme->border_bl);
    lua_setfield(L, -2, "border_bl");
    M_PushColor(L, theme->border_br);
    lua_setfield(L, -2, "border_br");
    M_PushRamp(L, theme->ramp, "ramp");
    M_PushRamp(L, theme->ramp_left, "ramp_left");
    M_PushRamp(L, theme->ramp_right, "ramp_right");
    return 1;
}

// trxc.ui.bar_scale() -> number
static int M_L_UIBarScale(lua_State *const L)
{
    lua_pushnumber(L, UI_Scaler_GetScale(UI_SCALER_TARGET_BAR));
    return 1;
}

static const luaL_Reg m_Module[] = {
    { "get_canvas_height", M_L_UICanvasHeight },
    { "get_canvas_width", M_L_UICanvasWidth },
    { "get_safe_bottom", M_L_UISafeBottom },
    { "get_safe_top", M_L_UISafeTop },
    { "get_safe_width", M_L_UISafeWidth },
    { "bar_theme", M_L_UIBarTheme },
    { "bar_scale", M_L_UIBarScale },
    { "reserve", M_L_UIReserve },
    { "slot_box", M_L_UISlotBox },
    { "measure_text", M_L_UIMeasureText },
    { "draw_text", M_L_UIDrawText },
    { "flat_quad", M_L_UIFlatQuad },
    { "gradient_quad", M_L_UIGradientQuad },
    { "to_screen", M_L_UIToScreen },
    { "to_canvas", M_L_UIToCanvas },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "ui", m_Module);
}

static void M_Shutdown(void)
{
    m_Drawing = false;
}

bool LUA_UI_IsPainting(void)
{
    return m_Painting;
}

void LUA_UI_SetPainting(const bool painting)
{
    m_Painting = painting;
}

void LUA_UI_SetDrawing(const bool drawing)
{
    m_Drawing = drawing;
}

bool LUA_UI_IsDrawing(void)
{
    return m_Drawing;
}

REGISTER_LUA_CAPI(.create = M_Create, .shutdown = M_Shutdown)
