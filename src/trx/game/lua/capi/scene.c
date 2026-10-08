#include <trx/core/colors.h>
#include <trx/core/math/const.h>
#include <trx/core/utils.h>
#include <trx/game/lua/events/scene.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>
#include <trx/game/matrix.h>
#include <trx/game/output/draw.h>

#include <lauxlib.h>

static void M_CheckPainting(lua_State *const L)
{
    if (!LUA_Scene_IsPainting()) {
        luaL_error(
            L, "trx.scene is only available from trx.events.on_scene_paint");
    }
}

static RGBA_8888 M_CheckColor(lua_State *const L, const int arg)
{
    const RGB_888 color = LUA_CheckValue(L, arg, TVT_RGB_888).as_rgb;
    int32_t alpha = luaL_optinteger(L, arg + 1, 255);
    CLAMP(alpha, 0, 255);
    return Color_RGBToRGBAEx(color, alpha);
}

// trxc.scene.box(min: trx.math.Vec3, max: trx.math.Vec3, color: trx.math.Color|string, alpha?: integer)
static int M_L_Box(lua_State *const L)
{
    M_CheckPainting(L);
    const XYZ_32 a = LUA_CheckXYZ(L, 1);
    const XYZ_32 b = LUA_CheckXYZ(L, 2);
    const RGBA_8888 color = M_CheckColor(L, 3);
    const BOUNDS_32 bounds = {
        .min = { MIN(a.x, b.x), MIN(a.y, b.y), MIN(a.z, b.z) },
        .max = { MAX(a.x, b.x), MAX(a.y, b.y), MAX(a.z, b.z) },
    };
    if (!Matrix_PushUnit()) {
        return 0;
    }
    Output_DrawCuboidEx(&bounds, color);
    Matrix_Pop();
    return 0;
}

// trxc.scene.sphere(centre: trx.math.Vec3, radius: trx.math.Distance, color: trx.math.Color|string, alpha?: integer)
static int M_L_Sphere(lua_State *const L)
{
    M_CheckPainting(L);
    const XYZ_32 centre = LUA_CheckXYZ(L, 1);
    int32_t radius = luaL_checkinteger(L, 2);
    CLAMPL(radius, 0);
    const RGBA_8888 color = M_CheckColor(L, 3);
    if (!Matrix_PushUnit()) {
        return 0;
    }
    Output_DrawSphereEx(centre, radius, color);
    Matrix_Pop();
    return 0;
}

static const luaL_Reg m_Module[] = {
    { "box", M_L_Box },
    { "sphere", M_L_Sphere },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "scene", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
