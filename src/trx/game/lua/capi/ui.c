#include <trx/config.h>
#include <trx/core/utils.h>
#include <trx/game/anims/types.h>
#include <trx/game/lua/field.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/struct.h>
#include <trx/game/lua/ui.h>
#include <trx/game/lua/utils.h>
#include <trx/game/objects/common.h>
#include <trx/game/output/const.h>
#include <trx/game/output/textures.h>
#include <trx/game/paths.h>
#include <trx/game/ui/common.h>
#include <trx/game/ui/draw.h>
#include <trx/game/ui/elements.h>
#include <trx/game/ui/elements/frame.h>
#include <trx/game/ui/keys.h>
#include <trx/game/ui/mesh_slots.h>
#include <trx/game/ui/regions.h>
#include <trx/game/ui/scaler.h>
#include <trx/game/ui/settings.h>
#include <trx/game/ui/text.h>

#include <lauxlib.h>
#include <math.h>

#define M_SLOT_POSE_GETTER(name_, member_)                                     \
    static bool M_GetSlot##name_(const void *const self, TRX_VALUE *const out) \
    {                                                                          \
        const UI_MESH_SLOT *const slot = self;                                 \
        *out = (TRX_VALUE) { .type = TVT_FLOAT, .as_num = slot->cur.member_ }; \
        return true;                                                           \
    }

M_SLOT_POSE_GETTER(X, x)
M_SLOT_POSE_GETTER(Y, y)
M_SLOT_POSE_GETTER(W, w)
M_SLOT_POSE_GETTER(H, h)

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

// Reads a trx.math.Color table as floats, defaulting alpha to opaque.
static RGBA_F M_CheckColorF(lua_State *const L, const int arg)
{
    const RGBA_8888 color = M_CheckColor(L, arg);
    return (RGBA_F) {
        .r = color.r / 255.0f,
        .g = color.g / 255.0f,
        .b = color.b / 255.0f,
        .a = color.a / 255.0f,
    };
}

// Resolves a script object and sprite number to a sprite index.
static int32_t M_CheckSpriteIdx(lua_State *const L)
{
    const OBJECT_ID object_id = LUA_CheckObjectID(L, 1);
    const OBJECT *const object = Object_Get(object_id);
    if (!object->loaded) {
        luaL_error(L, "object %d is not loaded", (int32_t)object_id);
    }
    const int32_t sprite_num = (int32_t)luaL_checkinteger(L, 2);
    if (sprite_num < 0 || sprite_num >= ABS(object->mesh_count)) {
        luaL_error(L, "sprite %d is out of range", sprite_num);
    }
    return object->mesh_idx + sprite_num;
}

// trxc.ui.mesh_bounds(object_id) -> min_x, min_y, min_z, max_x, max_y, max_z
static int M_L_UIMeshBounds(lua_State *const L)
{
    const OBJECT_ID object_id = LUA_CheckObjectID(L, 1);
    const OBJECT *const object = Object_Get(object_id);
    if (!object->loaded) {
        return luaL_error(L, "object %d is not loaded", (int32_t)object_id);
    }
    const ANIM *const anim = Object_GetAnim(object, 0);
    if (anim == nullptr || anim->frame_ptr == nullptr) {
        return luaL_error(L, "the object has no frame to measure");
    }
    const BOUNDS_16 bounds = anim->frame_ptr->bounds;
    lua_pushinteger(L, bounds.min.x);
    lua_pushinteger(L, bounds.min.y);
    lua_pushinteger(L, bounds.min.z);
    lua_pushinteger(L, bounds.max.x);
    lua_pushinteger(L, bounds.max.y);
    lua_pushinteger(L, bounds.max.z);
    return 6;
}

// trxc.ui.sprite_count(object_id) -> number
static int M_L_UISpriteCount(lua_State *const L)
{
    const OBJECT *const object = Object_Get(LUA_CheckObjectID(L, 1));
    lua_pushinteger(L, object->loaded ? ABS(object->mesh_count) : 0);
    return 1;
}

// trxc.ui.sprite_bounds(object_id, sprite_num) -> x0, y0, x1, y1
static int M_L_UISpriteBounds(lua_State *const L)
{
    const SPRITE_TEXTURE *const sprite =
        Output_GetSpriteTexture(M_CheckSpriteIdx(L));
    if (sprite == nullptr) {
        return luaL_error(L, "the sprite has no texture");
    }
    lua_pushnumber(L, sprite->x0);
    lua_pushnumber(L, sprite->y0);
    lua_pushnumber(L, sprite->x1);
    lua_pushnumber(L, sprite->y1);
    return 4;
}

// trxc.ui.sprite(object_id, sprite_num, x, y, z, scale, color)
static int M_L_UISprite(lua_State *const L)
{
    M_CheckPainting(L);
    const int32_t sprite_idx = M_CheckSpriteIdx(L);
    const int32_t x = lroundf(UI_ScaleX((float)luaL_checknumber(L, 3)));
    const int32_t y = lroundf(UI_ScaleY((float)luaL_checknumber(L, 4)));
    const int32_t z = (int32_t)luaL_optinteger(L, 5, 0);
    const int32_t scale =
        lroundf(UI_ScaleX((float)luaL_optnumber(L, 6, 1.0)) * PHD_ONE);
    const RGBA_F color = M_CheckColorF(L, 7);
    const RGBA_F colors[4] = { color, color, color, color };
    UI_ScheduleDrawScreenSprite(x, y, z, scale, scale, sprite_idx, colors);
    return 0;
}

static bool M_GetSlotRotY(const void *const self, TRX_VALUE *const out)
{
    const UI_MESH_SLOT *const slot = self;
    *out = (TRX_VALUE) { .type = TVT_S32, .as_int = slot->cur.rot_y };
    return true;
}

// clang-format off
static const FIELD_DESC m_MeshSlotFields[] = {
    FIELD_RO(UI_MESH_SLOT, object_id),
    FIELD_RO(UI_MESH_SLOT, visible),
    FIELD_FN("x", TVT_FLOAT, M_GetSlotX, nullptr),
    FIELD_FN("y", TVT_FLOAT, M_GetSlotY, nullptr),
    FIELD_FN("w", TVT_FLOAT, M_GetSlotW, nullptr),
    FIELD_FN("h", TVT_FLOAT, M_GetSlotH, nullptr),
    FIELD_FN("rot_y", TVT_S32, M_GetSlotRotY, nullptr),
};
// clang-format on

TYPE_DEFINE(UI_MESH_SLOT, m_MeshSlotFields)

static void *M_ResolveMeshSlot(const LUA_STRUCT_REF *const ref)
{
    return UI_MeshSlot_Resolve(ref->handle);
}

// slot:move{ object = ..., x = ..., y = ..., w = ..., h = ..., rot_y = ... }
static int M_L_UIMeshSlotMove(lua_State *const L)
{
    LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, &TYPE_UI_MESH_SLOT);
    LUA_Struct_Deref(L, ref);
    luaL_checktype(L, 2, LUA_TTABLE);
    lua_getfield(L, 2, "object");
    const OBJECT_ID object_id = LUA_CheckObjectID(L, -1);
    lua_pop(L, 1);
    UI_MeshSlot_Move(
        ref->handle, object_id,
        (UI_MESH_POSE) {
            .x = M_OptNumberField(L, 2, "x", 0.0f),
            .y = M_OptNumberField(L, 2, "y", 0.0f),
            .w = M_OptNumberField(L, 2, "w", 0.0f),
            .h = M_OptNumberField(L, 2, "h", 0.0f),
            .rot_y = (int32_t)M_OptNumberField(L, 2, "rot_y", 0.0f),
        });
    return 0;
}

// slot:hide()
static int M_L_UIMeshSlotHide(lua_State *const L)
{
    LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, &TYPE_UI_MESH_SLOT);
    LUA_Struct_Deref(L, ref);
    UI_MeshSlot_Hide(ref->handle);
    return 0;
}

// slot:release()
static int M_L_UIMeshSlotRelease(lua_State *const L)
{
    const LUA_STRUCT_REF *const ref =
        LUA_Struct_CheckRef(L, 1, &TYPE_UI_MESH_SLOT);
    UI_MeshSlot_Release(ref->handle);
    return 0;
}

// trxc.ui.mesh_slot() -> UI_MESH_SLOT handle or nil
static int M_L_UIMeshSlot(lua_State *const L)
{
    const TRX_HANDLE handle = UI_MeshSlot_Acquire();
    if (UI_MeshSlot_Resolve(handle) == nullptr) {
        lua_pushnil(L);
        return 1;
    }
    LUA_Struct_Push(L, &TYPE_UI_MESH_SLOT, M_ResolveMeshSlot, handle);
    return 1;
}

static const luaL_Reg m_MeshSlotMethods[] = {
    { "move", M_L_UIMeshSlotMove },
    { "hide", M_L_UIMeshSlotHide },
    { "release", M_L_UIMeshSlotRelease },
    { nullptr, nullptr },
};

// trxc.ui.gradient_sprite(object_id, sprite_num, x, y, z, scale, tl, tr, bl,
// br)
static int M_L_UIGradientSprite(lua_State *const L)
{
    M_CheckPainting(L);
    const int32_t sprite_idx = M_CheckSpriteIdx(L);
    const int32_t x = lroundf(UI_ScaleX((float)luaL_checknumber(L, 3)));
    const int32_t y = lroundf(UI_ScaleY((float)luaL_checknumber(L, 4)));
    const int32_t z = (int32_t)luaL_optinteger(L, 5, 0);
    const int32_t scale =
        lroundf(UI_ScaleX((float)luaL_optnumber(L, 6, 1.0)) * PHD_ONE);
    const RGBA_F colors[4] = {
        M_CheckColorF(L, 7),
        M_CheckColorF(L, 8),
        M_CheckColorF(L, 9),
        M_CheckColorF(L, 10),
    };
    UI_ScheduleDrawScreenSprite(x, y, z, scale, scale, sprite_idx, colors);
    return 0;
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

// trxc.ui.panel(x, y, z, w, h, style)
static int M_L_UIPanel(lua_State *const L)
{
    M_CheckPainting(L);
    const lua_Integer style = luaL_checkinteger(L, 6);
    if (style < 0 || style > UI_FRAME_OUTLINE_ONLY) {
        return luaL_error(L, "unknown frame style");
    }

    const float x = (float)luaL_checknumber(L, 1);
    const float y = (float)luaL_checknumber(L, 2);
    const int32_t z = (int32_t)luaL_optinteger(L, 3, 0);
    const int32_t x0 = lroundf(UI_ScaleX(x));
    const int32_t y0 = lroundf(UI_ScaleY(y));
    const int32_t w =
        (int32_t)lroundf(UI_ScaleX(x + (float)luaL_checknumber(L, 4))) - x0;
    const int32_t h =
        (int32_t)lroundf(UI_ScaleY(y + (float)luaL_checknumber(L, 5))) - y0;

    const UI_STYLE ui_style = g_Config.ui.menu_style;
    const TEXT_STYLE text_style = UI_Frame_GetTextStyle((UI_FRAME_STYLE)style);
    if (UI_Frame_HasBackground((UI_FRAME_STYLE)style)) {
        UI_ScheduleDrawTextBackground(ui_style, x0, y0, z, w, h, text_style);
    }
    UI_ScheduleDrawTextOutline(ui_style, x0, y0, z, w, h, text_style);
    return 0;
}

// trxc.ui.push_text_scale(factor)
static int M_L_UIPushTextScale(lua_State *const L)
{
    UI_Scaler_PushTextScale((float)luaL_checknumber(L, 1));
    return 0;
}

// trxc.ui.pop_text_scale()
static int M_L_UIPopTextScale(lua_State *const L)
{
    UI_Scaler_PopTextScale();
    return 0;
}

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

// trxc.ui.image(path, x, y, w, h, opacity) -> boolean
static int M_L_UIImage(lua_State *const L)
{
    M_CheckPainting(L);
    const char *const path = GamePath_PeekResolve(
        GAME_DYNAMIC_PATH_IMAGE_FILE, luaL_checkstring(L, 1));
    if (path == nullptr) {
        lua_pushboolean(L, false);
        return 1;
    }

    const float x = (float)luaL_checknumber(L, 2);
    const float y = (float)luaL_checknumber(L, 3);
    const float w = (float)luaL_checknumber(L, 4);
    const float h = (float)luaL_checknumber(L, 5);
    UI_ScheduleDrawImage(
        path, lroundf(UI_ScaleX(x)), lroundf(UI_ScaleY(y)),
        lroundf(UI_ScaleX(x + w)), lroundf(UI_ScaleY(y + h)),
        (float)luaL_optnumber(L, 6, 1.0));
    lua_pushboolean(L, true);
    return 1;
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

// trxc.ui.drawn_text_scale() -> number
static int M_L_UIDrawnTextScale(lua_State *const L)
{
    lua_pushnumber(L, UI_Scaler_GetTextScale());
    return 1;
}

// trxc.ui.text_scale() -> number
static int M_L_UITextScale(lua_State *const L)
{
    lua_pushnumber(L, UI_Scaler_GetScale(UI_SCALER_TARGET_TEXT));
    return 1;
}

// trxc.ui.get_clipboard() -> string
static int M_L_UIGetClipboard(lua_State *const L)
{
    lua_pushstring(L, UI_GetClipboardText());
    return 1;
}

// trxc.ui.set_clipboard(text)
static int M_L_UISetClipboard(lua_State *const L)
{
    const char *const text = luaL_checkstring(L, 1);
    RESULT result = UI_SetClipboardText(text);
    if (!IS_OK(result)) {
        lua_pushstring(
            L, result.msg != nullptr ? result.msg : "the clipboard refused it");
        IGNORE(result);
        return lua_error(L);
    }
    return 0;
}

static const luaL_Reg m_Module[] = {
    { "get_canvas_height", M_L_UICanvasHeight },
    { "get_canvas_width", M_L_UICanvasWidth },
    { "get_clipboard", M_L_UIGetClipboard },
    { "set_clipboard", M_L_UISetClipboard },
    { "get_safe_bottom", M_L_UISafeBottom },
    { "get_safe_top", M_L_UISafeTop },
    { "get_safe_width", M_L_UISafeWidth },
    { "bar_theme", M_L_UIBarTheme },
    { "bar_scale", M_L_UIBarScale },
    { "text_scale", M_L_UITextScale },
    { "drawn_text_scale", M_L_UIDrawnTextScale },
    { "reserve", M_L_UIReserve },
    { "slot_box", M_L_UISlotBox },
    { "measure_text", M_L_UIMeasureText },
    { "draw_text", M_L_UIDrawText },
    { "flat_quad", M_L_UIFlatQuad },
    { "panel", M_L_UIPanel },
    { "push_text_scale", M_L_UIPushTextScale },
    { "pop_text_scale", M_L_UIPopTextScale },
    { "gradient_quad", M_L_UIGradientQuad },
    { "image", M_L_UIImage },
    { "sprite", M_L_UISprite },
    { "sprite_bounds", M_L_UISpriteBounds },
    { "sprite_count", M_L_UISpriteCount },
    { "mesh_slot", M_L_UIMeshSlot },
    { "mesh_bounds", M_L_UIMeshBounds },
    { "gradient_sprite", M_L_UIGradientSprite },
    { "to_screen", M_L_UIToScreen },
    { "to_canvas", M_L_UIToCanvas },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "ui", m_Module);
    LUA_Struct_Register(L, &TYPE_UI_MESH_SLOT, m_MeshSlotMethods);
}

static void M_Shutdown(void)
{
    m_Drawing = false;
    m_Painting = false;
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
