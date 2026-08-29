#include <trx/core/utils.h>
#include <trx/game/level/settings.h>
#include <trx/game/lua/field.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/struct.h>
#include <trx/game/lua/utils.h>
#include <trx/game/output/lights.h>
#include <trx/game/output/lights/fog_bulbs.h>
#include <trx/game/rooms/common.h>
#include <trx/game/spawn.h>

#include <lauxlib.h>

typedef struct {
    XYZ_32 pos;
    RGB_888 color;
} M_COMMON;

typedef struct {
    XYZ_32 pos;
    int16_t room_num;
    int16_t strength;
    int16_t angle;
} M_BLOOD;

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

static M_BLOOD M_ReadBlood(lua_State *const L)
{
    const XYZ_32 pos = {
        .x = luaL_checkinteger(L, 1),
        .y = luaL_checkinteger(L, 2),
        .z = luaL_checkinteger(L, 3),
    };
    const int16_t room_num = Room_GetIndexFromPos(pos);
    if (room_num == NO_ROOM) {
        luaL_error(L, "position is outside the level");
    }

    int32_t strength = luaL_checkinteger(L, 4);
    CLAMP(strength, 1, 255);
    return (M_BLOOD) {
        .pos = pos,
        .room_num = room_num,
        .strength = strength,
        .angle = (int16_t)luaL_checkinteger(L, 5),
    };
}

// trxc.fx.blood(x, y, z, strength, angle)
static int M_L_Blood(lua_State *const L)
{
    const M_BLOOD blood = M_ReadBlood(L);
    Spawn_Blood(
        blood.pos.x, blood.pos.y, blood.pos.z, blood.strength, blood.angle,
        blood.room_num);
    return 0;
}

// trxc.fx.blood_bath(x, y, z, strength, angle, count)
static int M_L_BloodBath(lua_State *const L)
{
    const M_BLOOD blood = M_ReadBlood(L);
    int32_t count = luaL_checkinteger(L, 6);
    CLAMP(count, 1, 255);
    Spawn_BloodBath(
        blood.pos.x, blood.pos.y, blood.pos.z, blood.strength, blood.angle,
        blood.room_num, count);
    return 0;
}

// A bulb with no color of its own has none to report: it is drawn in the fog
// color in force.
static bool M_GetBulbColor(const void *const self, TRX_VALUE *const out)
{
    const FOG_BULB *const bulb = self;
    if (!bulb->has_own_color) {
        return false;
    }
    *out = (TRX_VALUE) { .type = TVT_RGB_888, .as_rgb = bulb->color };
    return true;
}

static const char *M_SetBulbColor(void *const self, const TRX_VALUE *const in)
{
    Output_FogBulbs_SetColor(self, in != nullptr ? &in->as_rgb : nullptr);
    return nullptr;
}

// Density reads as a share of the way to opaque, as the level data writes it,
// so a value outside the byte it is stored in is refused rather than folded.
static const char *M_SetBulbDensity(void *const self, const TRX_VALUE *const in)
{
    if (in->as_int < 0 || in->as_int > 255) {
        return "density runs from 0 to 255";
    }
    FOG_BULB *const bulb = self;
    bulb->density = in->as_int;
    return nullptr;
}

// clang-format off
static const FIELD_DESC m_BulbFields[] = {
    FIELD_FN_NULLABLE("color", TVT_RGB_888, M_GetBulbColor, M_SetBulbColor),
    FIELD_SET(FOG_BULB, density, M_SetBulbDensity),

    FIELD_RO(FOG_BULB, pos),
    FIELD_RO(FOG_BULB, radius),
};
// clang-format on

TYPE_DEFINE(FOG_BULB, m_BulbFields)

static void *M_ResolveBulb(const LUA_STRUCT_REF *const ref)
{
    return Output_FogBulbs_FromHandle(ref->handle);
}

// trxc.fx.fog_bulb_count() -> int
static int M_L_FogBulbCount(lua_State *const L)
{
    lua_pushinteger(L, Output_FogBulbs_GetStaticCount());
    return 1;
}

// trxc.fx.get_fog_bulb(index) -> FogBulb or nil
static int M_L_GetFogBulb(lua_State *const L)
{
    int32_t idx;
    if (!LUA_CheckBoundedInt(
            L, 1, 0, Output_FogBulbs_GetStaticCount() - 1, &idx)) {
        lua_pushnil(L);
        return 1;
    }
    LUA_Struct_Push(
        L, &TYPE_FOG_BULB, M_ResolveBulb, Output_FogBulbs_GetStaticHandle(idx));
    return 1;
}

// trxc.fx.get_fog_bulb_room(bulb) -> room number or nil
static int M_L_GetFogBulbRoom(lua_State *const L)
{
    LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, &TYPE_FOG_BULB);
    const FOG_BULB *const bulb = LUA_Struct_Deref(L, ref);
    LUA_PushOptIndex(L, bulb->room_num, NO_ROOM);
    return 1;
}

// trxc.fx.get_fog_color() -> color or nil
static int M_L_GetFogColor(lua_State *const L)
{
    const RGB_888 *const color = Level_GetFogColorOverride();
    if (color == nullptr) {
        lua_pushnil(L);
        return 1;
    }
    LUA_PushValue(L, &(TRX_VALUE) { .type = TVT_RGB_888, .as_rgb = *color });
    return 1;
}

// trxc.fx.set_fog_color(color or nil)
static int M_L_SetFogColor(lua_State *const L)
{
    if (lua_isnoneornil(L, 1)) {
        Level_ResetFogColorOverride();
        return 0;
    }
    const TRX_VALUE value = LUA_CheckValue(L, 1, TVT_RGB_888);
    Level_SetFogColorOverride(&value.as_rgb);
    return 0;
}

static const luaL_Reg m_Module[] = {
    { "blood", M_L_Blood },
    { "blood_bath", M_L_BloodBath },
    { "emit_light", M_L_EmitLight },
    { "emit_fog", M_L_EmitFog },
    { "fog_bulb_count", M_L_FogBulbCount },
    { "get_fog_bulb", M_L_GetFogBulb },
    { "get_fog_bulb_room", M_L_GetFogBulbRoom },
    { "get_fog_color", M_L_GetFogColor },
    { "set_fog_color", M_L_SetFogColor },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "fx", m_Module);
    LUA_Struct_Register(L, &TYPE_FOG_BULB, nullptr);

    LUA_GetModule(L, "fx");
    lua_pushinteger(L, OUTPUT_MAX_PENDING_LIGHTS);
    lua_setfield(L, -2, "MAX_LIGHTS");
    lua_pushinteger(L, OUTPUT_MAX_FOG_BULBS);
    lua_setfield(L, -2, "MAX_FOG");
    lua_pop(L, 1);
}

REGISTER_LUA_CAPI(.create = M_Create)
