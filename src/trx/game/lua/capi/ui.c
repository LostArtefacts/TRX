#include <trx/config.h>
#include <trx/core/memory.h>
#include <trx/core/utils.h>
#include <trx/game/anims/types.h>
#include <trx/game/interpolation.h>
#include <trx/game/lua/events/ui.h>
#include <trx/game/lua/field.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/struct.h>
#include <trx/game/lua/utils/args.h>
#include <trx/game/lua/utils/module.h>
#include <trx/game/objects/common.h>
#include <trx/game/output/const.h>
#include <trx/game/output/textures.h>
#include <trx/game/paths.h>
#include <trx/game/ui/common.h>
#include <trx/game/ui/draw.h>
#include <trx/game/ui/frame.h>
#include <trx/game/ui/keys.h>
#include <trx/game/ui/regions.h>
#include <trx/game/ui/scaler.h>
#include <trx/game/ui/settings.h>
#include <trx/game/ui/text.h>
#include <trx/game/viewport.h>

#include <lauxlib.h>
#include <math.h>

static bool m_Reserving = false;
static bool m_Painting = false;

static void M_CheckReserving(lua_State *const L)
{
    if (!m_Reserving) {
        luaL_error(
            L, "trx.ui.primitive.reserve works only during trx.ui.on_reserve");
    }
}

// trxc.ui.get_canvas_width(): number
static int M_L_UICanvasWidth(lua_State *const L)
{
    lua_pushnumber(L, UI_GetCanvasWidth());
    return 1;
}

// trxc.ui.get_canvas_height(): number
static int M_L_UICanvasHeight(lua_State *const L)
{
    lua_pushnumber(L, UI_GetCanvasHeight());
    return 1;
}

// trxc.ui.get_screen_width(): number
static int M_L_UIScreenWidth(lua_State *const L)
{
    lua_pushnumber(
        L,
        UI_Scaler_CalcInverse(
            Viewport_GetWidth(VIEWPORT_UI), UI_SCALER_TARGET_GENERIC));
    return 1;
}

// trxc.ui.get_screen_height(): number
static int M_L_UIScreenHeight(lua_State *const L)
{
    lua_pushnumber(
        L,
        UI_Scaler_CalcInverse(
            Viewport_GetHeight(VIEWPORT_UI), UI_SCALER_TARGET_GENERIC));
    return 1;
}

// trxc.ui.get_safe_width(): number
static int M_L_UISafeWidth(lua_State *const L)
{
    lua_pushnumber(L, UI_GetSafeCanvasWidth());
    return 1;
}

// trxc.ui.get_working_top(): number
static int M_L_UIWorkingTop(lua_State *const L)
{
    lua_pushnumber(L, UI_GetWorkingCanvasTop());
    return 1;
}

// trxc.ui.get_working_bottom(): number
static int M_L_UIWorkingBottom(lua_State *const L)
{
    lua_pushnumber(L, UI_GetWorkingCanvasBottom());
    return 1;
}

// trxc.ui.get_safe_top(width?: number): number
static int M_L_UISafeTop(lua_State *const L)
{
    lua_pushnumber(
        L, UI_GetSafeCanvasTopFor((float)luaL_optnumber(L, 1, -1.0)));
    return 1;
}

// trxc.ui.get_safe_bottom(width?: number): number
static int M_L_UISafeBottom(lua_State *const L)
{
    lua_pushnumber(
        L, UI_GetSafeCanvasBottomFor((float)luaL_optnumber(L, 1, -1.0)));
    return 1;
}

static void M_CheckPainting(lua_State *const L)
{
    if (!LUA_UI_IsPainting()) {
        luaL_error(L, "only available while a scene is being painted");
    }
}

// Keeps a z between the near and far planes of the UI view, so that drawing
// pushed too far stays visible at the nearest or farthest depth.
static int32_t M_ClampZ(int64_t z)
{
    CLAMP(z, 0, OUTPUT_UI_FAR_Z - OUTPUT_UI_NEAR_Z - 1);
    return (int32_t)z;
}

static int32_t M_OptZ(lua_State *const L, const int arg)
{
    return M_ClampZ(luaL_optinteger(L, arg, 0));
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

// Reads the colour of a light, keeping a channel's fraction: a light may be a
// shade no whole byte gives.
static RGB_F M_CheckLightColor(lua_State *const L, const int arg)
{
    luaL_checktype(L, arg, LUA_TTABLE);
    RGB_F color;
    float *const channels[] = { &color.r, &color.g, &color.b };
    const char *const keys[] = { "r", "g", "b" };
    for (int32_t i = 0; i < 3; i++) {
        lua_getfield(L, arg, keys[i]);
        double value = luaL_optnumber(L, -1, 0.0);
        lua_pop(L, 1);
        CLAMP(value, 0.0, 255.0);
        *channels[i] = (float)(value / 255.0);
    }
    return color;
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

// trxc.ui.mesh_bounds(object: trx.catalog.objects): trx.math.Distance?, trx.math.Distance, trx.math.Distance, trx.math.Distance, trx.math.Distance, trx.math.Distance
// The returns are min x, y, z, then max x, y, z.
static int M_L_UIMeshBounds(lua_State *const L)
{
    const OBJECT_ID object_id = LUA_CheckObjectID(L, 1);
    const OBJECT *const object = Object_Get(object_id);
    if (!object->loaded) {
        return luaL_error(L, "object %d is not loaded", (int32_t)object_id);
    }
    const ANIM *const anim = Object_GetAnim(object, 0);
    if (anim == nullptr || anim->frame_ptr == nullptr) {
        lua_pushnil(L);
        return 1;
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

// trxc.ui.sprite_count(object: trx.catalog.objects): integer
static int M_L_UISpriteCount(lua_State *const L)
{
    const OBJECT *const object = Object_Get(LUA_CheckObjectID(L, 1));
    const bool is_sprite = object->loaded && object->mesh_count < 0;
    lua_pushinteger(L, is_sprite ? -object->mesh_count : 0);
    return 1;
}

// trxc.ui.sprite_bounds(object: trx.catalog.objects, sprite_num: integer): number, number, number, number
// The returns are x0, y0, x1, y1.
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

// trxc.ui.sprite(object: trx.catalog.objects, sprite_num: integer, x: number, y: number, z?: integer, scale?: number, color: trx.math.Color|table)
static int M_L_UISprite(lua_State *const L)
{
    M_CheckPainting(L);
    const int32_t sprite_idx = M_CheckSpriteIdx(L);
    const int32_t x = lroundf(UI_ScaleX((float)luaL_checknumber(L, 3)));
    const int32_t y = lroundf(UI_ScaleY((float)luaL_checknumber(L, 4)));
    const int32_t z = M_OptZ(L, 5);
    const int32_t scale =
        lroundf(UI_ScaleX((float)luaL_optnumber(L, 6, 1.0)) * PHD_ONE);
    const RGBA_F color = M_CheckColorF(L, 7);
    const RGBA_F colors[4] = { color, color, color, color };
    UI_ScheduleDrawScreenSprite(x, y, z, scale, scale, sprite_idx, colors);
    return 0;
}

// The light a script gives a model: an ambient colour and a colour for each
// mesh it names, counted from 0.
// Reads a number field of a table, or the fallback where it is absent.
static float M_OptNumberField(
    lua_State *const L, const int arg, const char *const key,
    const float fallback)
{
    const int table = lua_absindex(L, arg);
    lua_getfield(L, table, key);
    const float value = (float)luaL_optnumber(L, -1, fallback);
    lua_pop(L, 1);
    return value;
}

static UI_MODEL_LIGHT M_CheckMeshLight(lua_State *const L, const int arg)
{
    luaL_checktype(L, arg, LUA_TTABLE);
    UI_MODEL_LIGHT light = {};
    lua_getfield(L, arg, "ambient");
    if (!lua_isnil(L, -1)) {
        light.ambient = M_CheckLightColor(L, lua_absindex(L, -1));
    }
    lua_pop(L, 1);
    lua_getfield(L, arg, "lights");
    if (!lua_isnil(L, -1)) {
        luaL_checktype(L, -1, LUA_TTABLE);
        const int lights = lua_absindex(L, -1);
        const int32_t count = (int32_t)lua_rawlen(L, lights);
        luaL_argcheck(
            L, count <= UI_MODEL_LIGHTS, arg,
            "a model takes up to three lights");
        for (int32_t i = 0; i < count; i++) {
            lua_rawgeti(L, lights, i + 1);
            luaL_checktype(L, -1, LUA_TTABLE);
            const int source = lua_absindex(L, -1);
            lua_getfield(L, source, "dir");
            light.lights[i].dir = LUA_CheckXYZ(L, lua_absindex(L, -1));
            lua_pop(L, 1);
            lua_getfield(L, source, "color");
            light.lights[i].color = M_CheckLightColor(L, lua_absindex(L, -1));
            lua_pop(L, 2);
        }
        light.light_count = count;
    }
    lua_pop(L, 1);
    lua_getfield(L, arg, "meshes");
    if (!lua_isnil(L, -1)) {
        luaL_checktype(L, -1, LUA_TTABLE);
        const int meshes = lua_absindex(L, -1);
        lua_pushnil(L);
        while (lua_next(L, meshes) != 0) {
            const lua_Integer idx = luaL_checkinteger(L, -2);
            luaL_argcheck(
                L, idx >= 0 && idx < 32, arg, "a mesh is counted from 0 to 31");
            light.colors[idx] = M_CheckLightColor(L, lua_absindex(L, -1));
            light.lit_mask |= 1u << idx;
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);
    return light;
}

// trxc.ui.draw_model(object: trx.catalog.objects, x: number, y: number, w: number, h: number, opts?: trx.ui.primitive.model.opts)
static int M_L_UIDrawModel(lua_State *const L)
{
    M_CheckPainting(L);
    const OBJECT_ID object_id = LUA_CheckObjectID(L, 1);
    const float x = (float)luaL_checknumber(L, 2);
    const float y = (float)luaL_checknumber(L, 3);
    const float w = (float)luaL_checknumber(L, 4);
    const float h = (float)luaL_checknumber(L, 5);

    XYZ_16 rot = {};
    // Read as an integer, because a mask of every mesh does not survive a
    // float.
    uint32_t mesh_mask = UINT32_MAX;
    UI_MODEL_LIGHT light;
    bool has_light = false;
    bool held = true;
    int16_t tilt = 0;
    if (!lua_isnoneornil(L, 6)) {
        luaL_checktype(L, 6, LUA_TTABLE);
        lua_getfield(L, 6, "rot");
        if (!lua_isnil(L, -1)) {
            luaL_checktype(L, -1, LUA_TTABLE);
            rot.x = (int16_t)(int32_t)M_OptNumberField(L, -1, "x", 0.0f);
            rot.y = (int16_t)(int32_t)M_OptNumberField(L, -1, "y", 0.0f);
            rot.z = (int16_t)(int32_t)M_OptNumberField(L, -1, "z", 0.0f);
        }
        lua_pop(L, 1);
        lua_getfield(L, 6, "mesh_mask");
        mesh_mask = (uint32_t)luaL_optinteger(L, -1, (lua_Integer)UINT32_MAX);
        lua_pop(L, 1);
        lua_getfield(L, 6, "light");
        if (!lua_isnil(L, -1)) {
            light = M_CheckMeshLight(L, lua_absindex(L, -1));
            has_light = true;
        }
        lua_pop(L, 1);
        lua_getfield(L, 6, "held");
        held = lua_isnil(L, -1) || lua_toboolean(L, -1);
        lua_pop(L, 1);
        lua_getfield(L, 6, "tilt");
        tilt = (int16_t)(int32_t)luaL_optnumber(L, -1, 0);
        lua_pop(L, 1);
    }

    const int32_t sx = lroundf(UI_ScaleX(x));
    const int32_t sy = lroundf(UI_ScaleY(y));
    UI_ScheduleDrawModel(
        object_id, mesh_mask, sx, sy, lroundf(UI_ScaleX(x + w)) - sx,
        lroundf(UI_ScaleY(y + h)) - sy, rot, held, tilt,
        has_light ? &light : nullptr);
    return 0;
}

// trxc.ui.draw_world_model(object: trx.catalog.objects, opts: trx.ui.primitive.world_model.opts)
static int M_L_UIDrawWorldModel(lua_State *const L)
{
    M_CheckPainting(L);
    const OBJECT_ID object_id = LUA_CheckObjectID(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);

    UI_MODEL_WORLD world = {};
    lua_getfield(L, 2, "camera");
    luaL_checktype(L, -1, LUA_TTABLE);
    const int camera = lua_absindex(L, -1);
    lua_getfield(L, camera, "pos");
    world.camera_pos = LUA_CheckXYZ(L, lua_absindex(L, -1));
    lua_pop(L, 1);
    lua_getfield(L, camera, "target");
    world.camera_target = LUA_CheckXYZ(L, lua_absindex(L, -1));
    lua_pop(L, 1);
    lua_getfield(L, camera, "fov");
    world.fov = (int16_t)luaL_checkinteger(L, -1);
    lua_pop(L, 2);

    lua_getfield(L, 2, "pose");
    luaL_checktype(L, -1, LUA_TTABLE);
    const int pose = lua_absindex(L, -1);
    const int32_t count = (int32_t)lua_rawlen(L, pose);
    luaL_argcheck(
        L, count <= UI_MODEL_STEPS, 2, "a pose takes up to sixteen steps");
    for (int32_t i = 0; i < count; i++) {
        lua_rawgeti(L, pose, i + 1);
        luaL_checktype(L, -1, LUA_TTABLE);
        const int step = lua_absindex(L, -1);
        lua_getfield(L, step, "turn");
        if (!lua_isnil(L, -1)) {
            world.steps[i].turn = true;
            world.steps[i].vec = LUA_CheckXYZ(L, lua_absindex(L, -1));
        } else {
            lua_pop(L, 1);
            lua_getfield(L, step, "move");
            world.steps[i].vec = LUA_CheckXYZ(L, lua_absindex(L, -1));
        }
        lua_pop(L, 2);
    }
    world.step_count = count;
    lua_pop(L, 1);

    lua_getfield(L, 2, "mesh_mask");
    const uint32_t mesh_mask =
        (uint32_t)luaL_optinteger(L, -1, (lua_Integer)UINT32_MAX);
    lua_pop(L, 1);
    UI_MODEL_LIGHT light;
    bool has_light = false;
    lua_getfield(L, 2, "light");
    if (!lua_isnil(L, -1)) {
        light = M_CheckMeshLight(L, lua_absindex(L, -1));
        has_light = true;
    }
    lua_pop(L, 1);

    UI_ScheduleDrawModelInWorld(
        object_id, mesh_mask, &world, has_light ? &light : nullptr);
    return 0;
}

// trxc.ui.darken(opacity: number)
static int M_L_UIDarken(lua_State *const L)
{
    M_CheckPainting(L);
    float opacity = (float)luaL_checknumber(L, 1);
    CLAMP(opacity, 0.0f, 1.0f);
    UI_ScheduleDrawDarken(opacity);
    return 0;
}

// trxc.ui.draw_background(path?: string, mono?: boolean, dim?: number): boolean
static int M_L_UIDrawBackground(lua_State *const L)
{
    M_CheckPainting(L);
    const char *const name = luaL_optstring(L, 1, nullptr);
    const char *const path = name != nullptr
        ? GamePath_PeekResolve(GAME_DYNAMIC_PATH_IMAGE_FILE, name)
        : nullptr;
    float dim = (float)luaL_optnumber(L, 3, 0.0);
    CLAMP(dim, 0.0f, 1.0f);
    UI_ScheduleDrawBackground(path, lua_toboolean(L, 2), dim);
    lua_pushboolean(L, path != nullptr);
    return 1;
}

// trxc.ui.interpolation_rate(): number
static int M_L_UIInterpolationRate(lua_State *const L)
{
    lua_pushnumber(L, Interpolation_GetRate());
    return 1;
}

// trxc.ui.gradient_sprite(object: trx.catalog.objects, sprite_num: integer, x: number, y: number, z?: integer, scale?: number, tl: trx.math.Color|table, tr: trx.math.Color|table, bl: trx.math.Color|table, br: trx.math.Color|table)
static int M_L_UIGradientSprite(lua_State *const L)
{
    M_CheckPainting(L);
    const int32_t sprite_idx = M_CheckSpriteIdx(L);
    const int32_t x = lroundf(UI_ScaleX((float)luaL_checknumber(L, 3)));
    const int32_t y = lroundf(UI_ScaleY((float)luaL_checknumber(L, 4)));
    const int32_t z = M_OptZ(L, 5);
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

// trxc.ui.reserve(region: trx.ui.Region, w: number, h: number): integer
static int M_L_UIReserve(lua_State *const L)
{
    M_CheckReserving(L);
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

// trxc.ui.slot_box(slot: integer): number?, number, number, number
// The returns are x, y, w, h.
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

// trxc.ui.measure_text(text: string, scale?: number): number, number
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

// trxc.ui.wrap_text(text: string, width: number, scale?: number): string
static int M_L_UIWrapText(lua_State *const L)
{
    char *wrapped = UI_Text_WordWrap(
        luaL_checkstring(L, 1), (float)luaL_optnumber(L, 3, 1.0),
        (float)luaL_checknumber(L, 2));
    lua_pushstring(L, wrapped != nullptr ? wrapped : "");
    Memory_FreePointer(&wrapped);
    return 1;
}

// trxc.ui.draw_text(text: string, x: number, y: number, scale?: number, z?: integer)
static int M_L_UIDrawText(lua_State *const L)
{
    M_CheckPainting(L);
    UI_Text_Draw(
        luaL_checkstring(L, 1), (float)luaL_checknumber(L, 2),
        (float)luaL_checknumber(L, 3),
        (UI_TEXT_SETTINGS) {
            .scale = (float)luaL_optnumber(L, 4, 1.0),
            .z = M_OptZ(L, 5),
        });
    return 0;
}

// trxc.ui.horizontal_line(x0: number, x1: number, y: number, z?: integer)
static int M_L_UIHorizontalLine(lua_State *const L)
{
    M_CheckPainting(L);
    UI_ScheduleDrawHorizontalLine(
        g_Config.ui.menu_style,
        lroundf(UI_ScaleX((float)luaL_checknumber(L, 1))),
        lroundf(UI_ScaleX((float)luaL_checknumber(L, 2))),
        lroundf(UI_ScaleY((float)luaL_checknumber(L, 3))), M_OptZ(L, 4));
    return 0;
}

// trxc.ui.panel(x: number, y: number, z?: integer, w: number, h: number, style: trx.ui.FrameStyle)
static int M_L_UIPanel(lua_State *const L)
{
    M_CheckPainting(L);
    const lua_Integer style = luaL_checkinteger(L, 6);
    if (style < 0 || style > UI_FRAME_HEADING_OUTLINE) {
        return luaL_error(L, "unknown frame style");
    }

    const float x = (float)luaL_checknumber(L, 1);
    const float y = (float)luaL_checknumber(L, 2);
    const int32_t z = M_OptZ(L, 3);
    // Cut to whole pixels as the engine's own frames are, so that a frame a
    // script draws lands where the engine would put it.
    const int32_t x0 = UI_ScaleX(x);
    const int32_t y0 = UI_ScaleY(y);
    const int32_t w = UI_ScaleX((float)luaL_checknumber(L, 4));
    const int32_t h = UI_ScaleY((float)luaL_checknumber(L, 5));

    const UI_STYLE ui_style = g_Config.ui.menu_style;
    const TEXT_STYLE text_style = UI_Frame_GetTextStyle((UI_FRAME_STYLE)style);
    if (UI_Frame_HasBackground((UI_FRAME_STYLE)style)) {
        UI_ScheduleDrawTextBackground(ui_style, x0, y0, z, w, h, text_style);
    }
    UI_ScheduleDrawTextOutline(ui_style, x0, y0, z, w, h, text_style);
    return 0;
}

// trxc.ui.push_text_scale(factor: number)
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

// trxc.ui.flat_quad(x: number, y: number, z?: integer, w: number, h: number, color: trx.math.Color|table)
static int M_L_UIFlatQuad(lua_State *const L)
{
    M_CheckPainting(L);
    const float x = (float)luaL_checknumber(L, 1);
    const float y = (float)luaL_checknumber(L, 2);
    const int32_t z = M_OptZ(L, 3);
    const int32_t x0 = lroundf(UI_ScaleX(x));
    const int32_t y0 = lroundf(UI_ScaleY(y));
    const int32_t w =
        (int32_t)lroundf(UI_ScaleX(x + (float)luaL_checknumber(L, 4))) - x0;
    const int32_t h =
        (int32_t)lroundf(UI_ScaleY(y + (float)luaL_checknumber(L, 5))) - y0;
    UI_ScheduleDrawScreenFlatQuad(x0, y0, z, w, h, M_CheckColor(L, 6));
    return 0;
}

// trxc.ui.gradient_quad(x: number, y: number, z?: integer, w: number, h: number, tl: trx.math.Color|table, tr: trx.math.Color|table, bl: trx.math.Color|table, br: trx.math.Color|table)
static int M_L_UIGradientQuad(lua_State *const L)
{
    M_CheckPainting(L);
    const float x = (float)luaL_checknumber(L, 1);
    const float y = (float)luaL_checknumber(L, 2);
    const int32_t z = M_OptZ(L, 3);
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

// trxc.ui.image(path: string, x: number, y: number, w: number, h: number, opacity?: number): boolean
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

// trxc.ui.to_screen(length: number): number
static int M_L_UIToScreen(lua_State *const L)
{
    lua_pushnumber(L, UI_ScaleY((float)luaL_checknumber(L, 1)));
    return 1;
}

// trxc.ui.to_canvas(pixels: number): number
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

// trxc.ui.bar_theme(type: trx.ui.BarType): { kind: "ps1"|"pc", basic_scale: number, border_light: { r: integer, g: integer, b: integer, a: integer }, border_dark: { r: integer, g: integer, b: integer, a: integer }, border_tl: { r: integer, g: integer, b: integer, a: integer }, border_tr: { r: integer, g: integer, b: integer, a: integer }, border_bl: { r: integer, g: integer, b: integer, a: integer }, border_br: { r: integer, g: integer, b: integer, a: integer }, ramp: table<integer, { r: integer, g: integer, b: integer, a: integer }>, ramp_left: table<integer, { r: integer, g: integer, b: integer, a: integer }>, ramp_right: table<integer, { r: integer, g: integer, b: integer, a: integer }> }?
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

// trxc.ui.bar_theme_key(type: trx.ui.BarType): lightuserdata?
static int M_L_UIBarThemeKey(lua_State *const L)
{
    const lua_Integer type = luaL_checkinteger(L, 1);
    if (type < 0 || type >= UI_BAR_NUMBER_OF) {
        return luaL_error(L, "unknown bar type");
    }
    const UI_BAR_THEME *const theme =
        UI_Settings_GetBarTheme((UI_BAR_TYPE)type);
    if (theme == nullptr) {
        lua_pushnil(L);
    } else {
        lua_pushlightuserdata(L, (void *)theme);
    }
    return 1;
}

// trxc.ui.is_ps1_bar(type: trx.ui.BarType): boolean
static int M_L_UIIsPS1Bar(lua_State *const L)
{
    const lua_Integer type = luaL_checkinteger(L, 1);
    if (type < 0 || type >= UI_BAR_NUMBER_OF) {
        return luaL_error(L, "unknown bar type");
    }
    lua_pushboolean(L, UI_Settings_IsBarLookPS1((UI_BAR_TYPE)type));
    return 1;
}

// trxc.ui.bar_scale(): number
static int M_L_UIBarScale(lua_State *const L)
{
    lua_pushnumber(L, UI_Scaler_GetScale(UI_SCALER_TARGET_BAR));
    return 1;
}

// trxc.ui.drawn_text_scale(): number
static int M_L_UIDrawnTextScale(lua_State *const L)
{
    lua_pushnumber(L, UI_Scaler_GetTextScale());
    return 1;
}

// trxc.ui.get_clipboard(): string
static int M_L_UIGetClipboard(lua_State *const L)
{
    lua_pushstring(L, UI_GetClipboardText());
    return 1;
}

// trxc.ui.set_clipboard(text: string)
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
    { "get_screen_height", M_L_UIScreenHeight },
    { "get_screen_width", M_L_UIScreenWidth },
    { "get_working_bottom", M_L_UIWorkingBottom },
    { "get_working_top", M_L_UIWorkingTop },
    { "get_safe_width", M_L_UISafeWidth },
    { "bar_theme", M_L_UIBarTheme },
    { "bar_theme_key", M_L_UIBarThemeKey },
    { "is_ps1_bar", M_L_UIIsPS1Bar },
    { "bar_scale", M_L_UIBarScale },
    { "drawn_text_scale", M_L_UIDrawnTextScale },
    { "reserve", M_L_UIReserve },
    { "slot_box", M_L_UISlotBox },
    { "measure_text", M_L_UIMeasureText },
    { "wrap_text", M_L_UIWrapText },
    { "draw_text", M_L_UIDrawText },
    { "flat_quad", M_L_UIFlatQuad },
    { "horizontal_line", M_L_UIHorizontalLine },
    { "panel", M_L_UIPanel },
    { "push_text_scale", M_L_UIPushTextScale },
    { "pop_text_scale", M_L_UIPopTextScale },
    { "gradient_quad", M_L_UIGradientQuad },
    { "image", M_L_UIImage },
    { "sprite", M_L_UISprite },
    { "sprite_bounds", M_L_UISpriteBounds },
    { "sprite_count", M_L_UISpriteCount },
    { "interpolation_rate", M_L_UIInterpolationRate },
    { "draw_background", M_L_UIDrawBackground },
    { "draw_model", M_L_UIDrawModel },
    { "draw_world_model", M_L_UIDrawWorldModel },
    { "darken", M_L_UIDarken },
    { "mesh_bounds", M_L_UIMeshBounds },
    { "gradient_sprite", M_L_UIGradientSprite },
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
    m_Reserving = false;
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

void LUA_UI_SetReserving(const bool reserving)
{
    m_Reserving = reserving;
}

bool LUA_UI_IsReserving(void)
{
    return m_Reserving;
}

REGISTER_LUA_CAPI(.create = M_Create, .shutdown = M_Shutdown)
