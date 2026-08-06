#include <trx/core/utils.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>
#include <trx/game/output/lights.h>

#include <lauxlib.h>

typedef struct {
    XYZ_32 pos;
    RGB_888 color;
} M_COMMON;

static int32_t M_ReadChannel(lua_State *const L, const int32_t idx)
{
    int32_t channel = luaL_checkinteger(L, idx);
    CLAMP(channel, 0, 255);
    return channel;
}

static M_COMMON M_ReadCommon(lua_State *const L)
{
    return (M_COMMON) {
        .pos = {
            .x = luaL_checkinteger(L, 1),
            .y = luaL_checkinteger(L, 2),
            .z = luaL_checkinteger(L, 3),
        },
        .color = {
            .r = M_ReadChannel(L, 4),
            .g = M_ReadChannel(L, 5),
            .b = M_ReadChannel(L, 6),
        },
    };
}

// trxc.fx.emit_light(x, y, z, r, g, b, radius)
static int M_L_EmitLight(lua_State *const L)
{
    const M_COMMON common = M_ReadCommon(L);
    int32_t radius = luaL_checkinteger(L, 7);
    CLAMPL(radius, 1);
    Output_AddDynamicLightRGB(common.pos, radius, common.color);
    return 0;
}

// trxc.fx.emit_fog(x, y, z, r, g, b, radius, density)
static int M_L_EmitFog(lua_State *const L)
{
    const M_COMMON common = M_ReadCommon(L);
    int32_t radius = luaL_checkinteger(L, 7);
    CLAMPL(radius, 1);
    int32_t density = luaL_checkinteger(L, 8);
    CLAMP(density, 0, 255);
    Output_AddFogBulb(common.pos, radius, density, common.color);
    return 0;
}

static const luaL_Reg m_Module[] = {
    { "emit_light", M_L_EmitLight },
    { "emit_fog", M_L_EmitFog },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "fx", m_Module);

    LUA_GetModule(L, "fx");
    lua_pushinteger(L, OUTPUT_MAX_PENDING_LIGHTS);
    lua_setfield(L, -2, "MAX_LIGHTS");
    lua_pushinteger(L, OUTPUT_MAX_FOG_BULBS);
    lua_setfield(L, -2, "MAX_FOG");
    lua_pop(L, 1);
}

REGISTER_LUA_CAPI(.create = M_Create)
