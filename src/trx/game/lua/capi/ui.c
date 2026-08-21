#include <trx/game/lua/registry.h>
#include <trx/game/lua/ui.h>
#include <trx/game/lua/utils.h>
#include <trx/game/ui/common.h>
#include <trx/game/ui/elements.h>
#include <trx/game/ui/settings.h>

#include <lauxlib.h>

static bool m_Drawing = false;

static void M_CheckDrawing(lua_State *const L)
{
    if (!m_Drawing) {
        luaL_error(L, "trx.ui is only available from trx.events.on_ui_draw");
    }
}

// Runs the body between a widget's begin and end. The end always runs, so an
// error inside the body leaves the scene balanced.
static int M_CallBody(lua_State *const L, const int arg, void (*end)(void))
{
    lua_pushvalue(L, arg);
    const int status = lua_pcall(L, 0, 0, 0);
    end();
    if (status != LUA_OK) {
        return lua_error(L);
    }
    return 0;
}

static lua_Integer M_GetEnumField(
    lua_State *const L, const int arg, const char *const key,
    const lua_Integer fallback, const lua_Integer count)
{
    lua_getfield(L, arg, key);
    const lua_Integer value = luaL_optinteger(L, -1, fallback);
    lua_pop(L, 1);
    if (value < 0 || value >= count) {
        luaL_error(L, "%s is not a value this setting takes", key);
    }
    return value;
}

static float M_GetNumberField(
    lua_State *const L, const int arg, const char *const key)
{
    lua_getfield(L, arg, key);
    const float value = (float)luaL_optnumber(L, -1, 0.0);
    lua_pop(L, 1);
    return value;
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

static UI_LABEL_SETTINGS M_GetLabelSettings(lua_State *const L, const int arg)
{
    UI_LABEL_SETTINGS settings = { .scale = 1.0f };
    if (lua_isnoneornil(L, arg)) {
        return settings;
    }
    luaL_checktype(L, arg, LUA_TTABLE);
    settings.scale = M_OptNumberField(L, arg, "scale", 1.0f);
    lua_getfield(L, arg, "z");
    settings.z = (int32_t)luaL_optinteger(L, -1, 0);
    lua_pop(L, 1);
    return settings;
}

// trxc.ui.label(text, settings)
static int M_L_UILabel(lua_State *const L)
{
    M_CheckDrawing(L);
    const char *const text = luaL_checkstring(L, 1);
    UI_LabelEx(text, M_GetLabelSettings(L, 2));
    return 0;
}

// trxc.ui.spacer(w, h)
static int M_L_UISpacer(lua_State *const L)
{
    M_CheckDrawing(L);
    UI_Spacer((float)luaL_checknumber(L, 1), (float)luaL_checknumber(L, 2));
    return 0;
}

// trxc.ui.measure(text, settings) -> w, h
static int M_L_UIMeasure(lua_State *const L)
{
    const char *const text = luaL_checkstring(L, 1);
    float w = 0.0f;
    float h = 0.0f;
    UI_Label_MeasureEx(text, &w, &h, M_GetLabelSettings(L, 2));
    lua_pushnumber(L, w);
    lua_pushnumber(L, h);
    return 2;
}

// trxc.ui.bar(settings)
static int M_L_UIBar(lua_State *const L)
{
    M_CheckDrawing(L);
    luaL_checktype(L, 1, LUA_TTABLE);

    const UI_BAR_SETTINGS settings = {
        .type = (UI_BAR_TYPE)M_GetEnumField(
            L, 1, "type", UI_BAR_LARA_HP, UI_BAR_NUMBER_OF),
        .w = (int32_t)M_OptNumberField(L, 1, "w", UI_BAR_WIDTH),
        .h = (int32_t)M_OptNumberField(L, 1, "h", UI_BAR_HEIGHT),
        .value = (int32_t)M_OptNumberField(L, 1, "value", 0.0f),
        .max_value = (int32_t)M_OptNumberField(L, 1, "max_value", 100.0f),
    };
    UI_Bar(settings);
    return 0;
}

// trxc.ui.stack(settings, body)
static int M_L_UIStack(lua_State *const L)
{
    M_CheckDrawing(L);
    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    UI_STACK_SETTINGS settings = {};
    lua_getfield(L, 1, "orientation");
    settings.orientation =
        (UI_STACK_ORIENTATION)luaL_optinteger(L, -1, UI_STACK_VERTICAL);
    lua_pop(L, 1);
    if (settings.orientation != UI_STACK_VERTICAL
        && settings.orientation != UI_STACK_HORIZONTAL) {
        return luaL_error(L, "unknown stack orientation");
    }

    lua_getfield(L, 1, "align");
    if (lua_istable(L, -1)) {
        const int align_idx = lua_gettop(L);
        settings.align.h = (UI_STACK_H_ALIGN)M_GetEnumField(
            L, align_idx, "h", UI_STACK_H_ALIGN_LEFT,
            UI_STACK_H_ALIGN_DISTRIBUTE + 1);
        settings.align.v = (UI_STACK_V_ALIGN)M_GetEnumField(
            L, align_idx, "v", UI_STACK_V_ALIGN_TOP,
            UI_STACK_V_ALIGN_DISTRIBUTE + 1);
    }
    lua_pop(L, 1);

    lua_getfield(L, 1, "spacing");
    if (lua_istable(L, -1)) {
        settings.spacing.h = M_GetNumberField(L, lua_gettop(L), "h");
        settings.spacing.v = M_GetNumberField(L, lua_gettop(L), "v");
    }
    lua_pop(L, 1);

    UI_BeginStackEx(settings);
    return M_CallBody(L, 2, UI_EndStack);
}

// trxc.ui.anchor(x, y, body)
static int M_L_UIAnchor(lua_State *const L)
{
    M_CheckDrawing(L);
    const float x = (float)luaL_checknumber(L, 1);
    const float y = (float)luaL_checknumber(L, 2);
    luaL_checktype(L, 3, LUA_TFUNCTION);
    UI_BeginAnchor(x, y);
    return M_CallBody(L, 3, UI_EndAnchor);
}

// trxc.ui.pad(settings, body)
static int M_L_UIPad(lua_State *const L)
{
    M_CheckDrawing(L);
    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    const float x = M_GetNumberField(L, 1, "x");
    const float y = M_GetNumberField(L, 1, "y");
    UI_BeginPadEx(
        M_OptNumberField(L, 1, "l", x), M_OptNumberField(L, 1, "r", x),
        M_OptNumberField(L, 1, "t", y), M_OptNumberField(L, 1, "b", y));
    return M_CallBody(L, 2, UI_EndPad);
}

// trxc.ui.hide(hidden, body)
static int M_L_UIHide(lua_State *const L)
{
    M_CheckDrawing(L);
    luaL_checktype(L, 1, LUA_TBOOLEAN);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    UI_BeginHide(lua_toboolean(L, 1));
    return M_CallBody(L, 2, UI_EndHide);
}

// trxc.ui.resize(settings, body)
static int M_L_UIResize(lua_State *const L)
{
    M_CheckDrawing(L);
    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    const UI_RESIZE_SETTINGS settings = {
        .w = M_OptNumberField(L, 1, "w", -1.0f),
        .h = M_OptNumberField(L, 1, "h", -1.0f),
        .align_h = M_GetNumberField(L, 1, "align_h"),
        .align_v = M_GetNumberField(L, 1, "align_v"),
    };
    UI_BeginResizeEx(settings);
    return M_CallBody(L, 2, UI_EndResize);
}

// trxc.ui.frame(style, body)
static int M_L_UIFrame(lua_State *const L)
{
    M_CheckDrawing(L);
    const lua_Integer style = luaL_checkinteger(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    if (style < 0 || style > UI_FRAME_OUTLINE_ONLY) {
        return luaL_error(L, "unknown frame style");
    }
    UI_BeginFrame((UI_FRAME_STYLE)style);
    return M_CallBody(L, 2, UI_EndFrame);
}

// trxc.ui.offset(x, y, body)
static int M_L_UIOffset(lua_State *const L)
{
    M_CheckDrawing(L);
    const float x = (float)luaL_checknumber(L, 1);
    const float y = (float)luaL_checknumber(L, 2);
    luaL_checktype(L, 3, LUA_TFUNCTION);
    UI_BeginOffset(x, y);
    return M_CallBody(L, 3, UI_EndOffset);
}

// trxc.ui.span(body)
static int M_L_UISpan(lua_State *const L)
{
    M_CheckDrawing(L);
    luaL_checktype(L, 1, LUA_TFUNCTION);
    UI_BeginSpan();
    return M_CallBody(L, 1, UI_EndSpan);
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

static const luaL_Reg m_Module[] = {
    { "anchor", M_L_UIAnchor },
    { "bar", M_L_UIBar },
    { "frame", M_L_UIFrame },
    { "get_canvas_height", M_L_UICanvasHeight },
    { "get_canvas_width", M_L_UICanvasWidth },
    { "get_safe_bottom", M_L_UISafeBottom },
    { "get_safe_top", M_L_UISafeTop },
    { "get_safe_width", M_L_UISafeWidth },
    { "hide", M_L_UIHide },
    { "label", M_L_UILabel },
    { "measure", M_L_UIMeasure },
    { "offset", M_L_UIOffset },
    { "pad", M_L_UIPad },
    { "resize", M_L_UIResize },
    { "spacer", M_L_UISpacer },
    { "span", M_L_UISpan },
    { "stack", M_L_UIStack },
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

void LUA_UI_SetDrawing(const bool drawing)
{
    m_Drawing = drawing;
}

bool LUA_UI_IsDrawing(void)
{
    return m_Drawing;
}

REGISTER_LUA_CAPI(.create = M_Create, .shutdown = M_Shutdown)
