#include <trx/core/colors.h>
#include <trx/core/utils.h>
#include <trx/game/effects.h>
#include <trx/game/fx.h>
#include <trx/game/fx/fire.h>
#include <trx/game/gun/registry.h>
#include <trx/game/items.h>
#include <trx/game/level/settings.h>
#include <trx/game/lua/field.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/struct.h>
#include <trx/game/lua/utils.h>
#include <trx/game/output/lights.h>
#include <trx/game/output/lights/fog_bulbs.h>
#include <trx/game/rooms/common.h>
#include <trx/game/sparks.h>
#include <trx/game/sparks/spawners.h>
#include <trx/game/spawn.h>
#include <trx/version.h>

#include <lauxlib.h>

// A spark's flags sit in one word rather than in a bitfield, so each public one
// is bridged as the boolean it stands for.
#define M_SPARK_FLAG(name_, bit_)                                              \
    static bool M_GetSpark##name_(                                             \
        const void *const self, TRX_VALUE *const out)                          \
    {                                                                          \
        const SPARK *const spark = self;                                       \
        *out = Value_Make_TVT_BOOL((spark->flags & (bit_)) != 0U);             \
        return true;                                                           \
    }                                                                          \
    static const char *M_SetSpark##name_(                                      \
        void *const self, const TRX_VALUE *const in)                           \
    {                                                                          \
        SPARK *const spark = self;                                             \
        if (in->as_bool) {                                                     \
            spark->flags |= (uint16_t)(bit_);                                  \
        } else {                                                               \
            spark->flags &= (uint16_t)~(bit_);                                 \
        }                                                                      \
        return nullptr;                                                        \
    }

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

M_SPARK_FLAG(Scales, SPARK_F_SCALE)
M_SPARK_FLAG(IsBlood, SPARK_F_BLOOD)
M_SPARK_FLAG(Rotates, SPARK_F_ROTATE)
M_SPARK_FLAG(IsOutside, SPARK_F_OUTSIDE)
M_SPARK_FLAG(UsesAltSprite, SPARK_F_ALT_SPRITE)
M_SPARK_FLAG(IsUnderwater, SPARK_F_UNDERWATER)
M_SPARK_FLAG(IsGreen, SPARK_F_GREEN)

// clang-format off
static const FIELD_DESC m_SparkFields[] = {
    FIELD(SPARK, life),
    FIELD(SPARK, s_life),
    FIELD(SPARK, pos),
    FIELD(SPARK, vel),
    FIELD(SPARK, size.width),
    FIELD(SPARK, size.height),
    FIELD(SPARK, src_size.width),
    FIELD(SPARK, src_size.height),
    FIELD(SPARK, dst_size.width),
    FIELD(SPARK, dst_size.height),
    FIELD(SPARK, color),
    FIELD(SPARK, src_color),
    FIELD(SPARK, dst_color),
    FIELD(SPARK, scalar),
    FIELD(SPARK, col_fade_speed),
    FIELD(SPARK, fade_to_black),
    FIELD(SPARK, gravity),
    FIELD(SPARK, max_y_vel),
    FIELD(SPARK, friction),
    FIELD_MODULAR(SPARK, rot_angle),
    FIELD(SPARK, rot_add),
    FIELD(SPARK, extras),
    FIELD(SPARK, node_num),
    FIELD_RO(SPARK, room_num),
    FIELD_RO(SPARK, draw_type),

    FIELD_FN("scales", TVT_BOOL, M_GetSparkScales, M_SetSparkScales),
    FIELD_FN("is_blood", TVT_BOOL, M_GetSparkIsBlood, M_SetSparkIsBlood),
    FIELD_FN("rotates", TVT_BOOL, M_GetSparkRotates, M_SetSparkRotates),
    FIELD_FN("is_outside", TVT_BOOL, M_GetSparkIsOutside, M_SetSparkIsOutside),
    FIELD_FN(
        "uses_alt_sprite", TVT_BOOL, M_GetSparkUsesAltSprite,
        M_SetSparkUsesAltSprite),
    FIELD_FN(
        "is_underwater", TVT_BOOL, M_GetSparkIsUnderwater,
        M_SetSparkIsUnderwater),
    FIELD_FN("is_green", TVT_BOOL, M_GetSparkIsGreen, M_SetSparkIsGreen),
};
// clang-format on

TYPE_DEFINE(SPARK, m_SparkFields)

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

// The room a world position falls in, or a raised error where it falls outside
// the level: an effect the engine puts in a room has nowhere to go without one.
static int16_t M_CheckRoom(lua_State *const L, const XYZ_32 pos)
{
    const int16_t room_num = Room_GetIndexFromPos(pos);
    if (room_num == NO_ROOM) {
        luaL_error(L, "position is outside the level");
    }
    return room_num;
}

static XYZ_32 M_ReadPos(lua_State *const L)
{
    return (XYZ_32) {
        .x = luaL_checkinteger(L, 1),
        .y = luaL_checkinteger(L, 2),
        .z = luaL_checkinteger(L, 3),
    };
}

static ITEM *M_CheckItem(lua_State *const L, const int arg)
{
    const int32_t item_num =
        LUA_CheckRange(L, arg, Item_GetTotalCount(), "item");
    return Item_Get(item_num);
}

// trxc.fx.explosion(x, y, z, with_sound)
static int M_L_Explosion(lua_State *const L)
{
    const XYZ_32 pos = M_ReadPos(L);
    Spawn_Explosion(pos, M_CheckRoom(L, pos), lua_toboolean(L, 4));
    return 0;
}

// trxc.fx.fire(x, y, z, size, fade)
static int M_L_Fire(lua_State *const L)
{
    const XYZ_32 pos = M_ReadPos(L);
    int32_t size = luaL_checkinteger(L, 4);
    CLAMP(size, 0, 2);
    FX_Fire_Add(pos, size, M_CheckRoom(L, pos), luaL_checkinteger(L, 5));
    return 0;
}

// trxc.fx.splash(item_num)
static int M_L_Splash(lua_State *const L)
{
    FX_Water_Splash(M_CheckItem(L, 1));
    return 0;
}

// trxc.fx.wade_splash(item_num, depth)
static int M_L_WadeSplash(lua_State *const L)
{
    FX_Water_WadeSplash(M_CheckItem(L, 1), luaL_checkinteger(L, 2));
    return 0;
}

// trxc.fx.ripple(x, y, z, size, slow, dark, blood, jitter)
static int M_L_Ripple(lua_State *const L)
{
    const XYZ_32 pos = M_ReadPos(L);
    int32_t size = luaL_checkinteger(L, 4);
    CLAMP(size, 1, 255);

    uint32_t flags = FX_RIPPLE_ACTIVE;
    if (lua_toboolean(L, 5)) {
        flags |= FX_RIPPLE_SLOW;
    }
    if (lua_toboolean(L, 6)) {
        flags |= FX_RIPPLE_DARK;
    }
    if (lua_toboolean(L, 7)) {
        flags |= FX_RIPPLE_BLOOD;
    }
    if (lua_toboolean(L, 8)) {
        flags |= FX_RIPPLE_JITTER;
    }
    FX_Water_SetupRipple(pos, size, flags);
    return 0;
}

// trxc.fx.small_splash(x, y, z, count)
static int M_L_SmallSplash(lua_State *const L)
{
    const XYZ_32 pos = M_ReadPos(L);
    int32_t count = luaL_checkinteger(L, 4);
    CLAMP(count, 1, 255);
    Sparks_TriggerSmallSplash(pos, count);
    return 0;
}

// trxc.fx.underwater_blood(x, y, z, size, dark)
static int M_L_UnderwaterBlood(lua_State *const L)
{
    const XYZ_32 pos = M_ReadPos(L);
    int32_t size = luaL_checkinteger(L, 4);
    CLAMP(size, 1, 255);
    if (lua_toboolean(L, 5)) {
        FX_Water_TriggerUnderwaterBloodD(pos, size);
    } else {
        FX_Water_TriggerUnderwaterBlood(pos, size);
    }
    return 0;
}

// trxc.fx.footprint(item_num, is_left_foot)
static int M_L_Footprint(lua_State *const L)
{
    FX_Footprint_Add(M_CheckItem(L, 1), lua_toboolean(L, 2));
    return 0;
}

// trxc.fx.knockback(x, y, z)
static int M_L_Knockback(lua_State *const L)
{
    FX_Ring_SpawnKnockBack(M_ReadPos(L));
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
    const TRX_VALUE *const color =
        Level_GetSettingOverride(LEVEL_SETTING_FOG_COLOR);
    if (color == nullptr) {
        lua_pushnil(L);
        return 1;
    }
    LUA_PushValue(L, color);
    return 1;
}

// trxc.fx.set_fog_color(color or nil)
static int M_L_SetFogColor(lua_State *const L)
{
    if (lua_isnoneornil(L, 1)) {
        Level_ResetSettingOverride(LEVEL_SETTING_FOG_COLOR);
        return 0;
    }
    const TRX_VALUE value = LUA_CheckValue(L, 1, TVT_RGB_888);
    SHOULD(Level_SetSettingOverride(LEVEL_SETTING_FOG_COLOR, &value));
    return 0;
}

static void *M_ResolveSpark(const LUA_STRUCT_REF *const ref)
{
    return Sparks_FromHandle(ref->handle);
}

static void M_PushSpark(lua_State *const L, SPARK *const spark)
{
    if (spark == nullptr) {
        lua_pushnil(L);
        return;
    }
    LUA_Struct_Push(L, &TYPE_SPARK, M_ResolveSpark, Sparks_GetHandle(spark));
}

static SPARK *M_CheckSpark(lua_State *const L, const int arg)
{
    LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, arg, &TYPE_SPARK);
    return LUA_Struct_Deref(L, ref);
}

// An optional integer in an options table, clamped to what the member holds.
static int32_t M_OptField(
    lua_State *const L, const int arg, const char *const key,
    const int32_t fallback, const int32_t lo, const int32_t hi)
{
    lua_getfield(L, arg, key);
    int32_t value = fallback;
    if (!lua_isnil(L, -1)) {
        value = (int32_t)luaL_checkinteger(L, -1);
        CLAMP(value, lo, hi);
    }
    lua_pop(L, 1);
    return value;
}

static bool M_OptFlagField(
    lua_State *const L, const int arg, const char *const key)
{
    lua_getfield(L, arg, key);
    const bool value = lua_toboolean(L, -1);
    lua_pop(L, 1);
    return value;
}

static RGB_888 M_OptColorField(
    lua_State *const L, const int arg, const char *const key,
    const RGB_888 fallback)
{
    lua_getfield(L, arg, key);
    RGB_888 color = fallback;
    if (!lua_isnil(L, -1)) {
        color = LUA_CheckValue(L, -1, TVT_RGB_888).as_rgb;
    }
    lua_pop(L, 1);
    return color;
}

static XYZ_32 M_OptXYZField(
    lua_State *const L, const int arg, const char *const key)
{
    lua_getfield(L, arg, key);
    const XYZ_32 value =
        lua_isnil(L, -1) ? (XYZ_32) {} : LUA_CheckXYZAt(L, lua_gettop(L), arg);
    lua_pop(L, 1);
    return value;
}

// trxc.fx.spawn_spark(opts) -> spark or nil
static int M_L_SpawnSpark(lua_State *const L)
{
    luaL_checktype(L, 1, LUA_TTABLE);

    // Everything the options table is refused over is read before a slot is
    // taken, so a raised error leaves no half-built spark alive in the pool.
    const int32_t sprite_type =
        M_OptField(L, 1, "sprite_type", SPARK_TYPE_PARTICLE, 0, INT8_MAX);
    const XYZ_32 pos = M_OptXYZField(L, 1, "pos");
    const int16_t room_num = M_CheckRoom(L, pos);

    SPARK *const spark =
        Sparks_InitialiseSpriteSpark((SPARK_SPRITE_TYPE)sprite_type);
    if (spark == nullptr) {
        lua_pushnil(L);
        return 1;
    }

    const int32_t life = M_OptField(L, 1, "life", 16, 1, UINT8_MAX);
    const RGB_888 color = M_OptColorField(L, 1, "color", COLOR_RGB_888_WHITE);

    spark->pos = pos;
    spark->room_num = (uint8_t)room_num;
    spark->vel = M_OptXYZField(L, 1, "vel");
    spark->life = life;
    spark->s_life = life;
    spark->src_color = color;
    spark->dst_color = M_OptColorField(L, 1, "end_color", color);
    spark->src_size.width = M_OptField(L, 1, "width", 4, 0, UINT8_MAX);
    spark->src_size.height = M_OptField(L, 1, "height", 4, 0, UINT8_MAX);
    spark->dst_size.width =
        M_OptField(L, 1, "end_width", spark->src_size.width, 0, UINT8_MAX);
    spark->dst_size.height =
        M_OptField(L, 1, "end_height", spark->src_size.height, 0, UINT8_MAX);
    spark->size = spark->src_size;
    spark->scalar = M_OptField(L, 1, "scalar", 2, 0, UINT8_MAX);
    spark->col_fade_speed = M_OptField(L, 1, "fade_speed", 8, 0, UINT8_MAX);
    spark->fade_to_black = M_OptField(L, 1, "fade_to_black", 8, 0, UINT8_MAX);
    spark->gravity = M_OptField(L, 1, "gravity", 0, INT16_MIN, INT16_MAX);
    spark->max_y_vel = M_OptField(L, 1, "max_y_vel", 0, INT8_MIN, INT8_MAX);
    spark->friction = M_OptField(L, 1, "friction", 0, 0, UINT8_MAX);
    spark->rot_angle = M_OptField(L, 1, "rot_angle", 0, 0, 0xFFF);
    spark->rot_add = M_OptField(L, 1, "rot_add", 0, INT8_MIN, INT8_MAX);
    spark->extras = M_OptField(L, 1, "extras", 0, 0, UINT8_MAX);
    spark->dynamic = -1;
    spark->draw_type = M_OptField(
        L, 1, "draw_type", DRAW_BLEND_ADD, 0, DRAW_REFLECTIVE_BLEND_ADD);

    if (M_OptFlagField(L, 1, "scales")) {
        spark->flags |= SPARK_F_SCALE;
    }
    if (M_OptFlagField(L, 1, "rotates")) {
        spark->flags |= SPARK_F_ROTATE;
    }
    if (M_OptFlagField(L, 1, "is_outside")) {
        spark->flags |= SPARK_F_OUTSIDE;
    }
    if (M_OptFlagField(L, 1, "uses_alt_sprite")) {
        spark->flags |= SPARK_F_ALT_SPRITE;
    }
    if (M_OptFlagField(L, 1, "is_underwater")) {
        spark->flags |= SPARK_F_UNDERWATER;
    }
    if (M_OptFlagField(L, 1, "is_green")) {
        spark->flags |= SPARK_F_GREEN;
    }

    Sparks_FinishSetup(spark);
    M_PushSpark(L, spark);
    return 1;
}

// trxc.fx.spark_max_count() -> int
static int M_L_SparkMaxCount(lua_State *const L)
{
    lua_pushinteger(L, Sparks_GetMaxCount());
    return 1;
}

// trxc.fx.get_spark(index) -> spark or nil
static int M_L_GetSpark(lua_State *const L)
{
    int32_t idx;
    if (!LUA_CheckBoundedInt(L, 1, 0, Sparks_GetMaxCount() - 1, &idx)) {
        lua_pushnil(L);
        return 1;
    }
    SPARK *const spark = Sparks_GetSpark(idx);
    M_PushSpark(L, spark->on ? spark : nullptr);
    return 1;
}

// trxc.fx.get_spark_world_pos(spark) -> pos
static int M_L_GetSparkWorldPos(lua_State *const L)
{
    LUA_PushXYZ(L, Sparks_GetWorldPos(M_CheckSpark(L, 1)));
    return 1;
}

// trxc.fx.get_spark_item(spark) -> item number or nil
static int M_L_GetSparkItem(lua_State *const L)
{
    const SPARK *const spark = M_CheckSpark(L, 1);
    if ((spark->flags & SPARK_F_ITEM) == 0U) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushinteger(L, spark->item_num);
    return 1;
}

// spark:kill()
static int M_L_SparkKill(lua_State *const L)
{
    SPARK *const spark = M_CheckSpark(L, 1);
    if (spark->dynamic != -1) {
        Sparks_FreeDynamic(spark->dynamic);
        spark->dynamic = -1;
    }
    spark->on = false;
    spark->life = 0;
    return 0;
}

// trxc.fx.get_smoke_wind() -> x, z
static int M_L_GetSmokeWind(lua_State *const L)
{
    const XZ_32 wind = Sparks_GetSmokeWind();
    lua_pushinteger(L, wind.x);
    lua_pushinteger(L, wind.z);
    return 2;
}

// trxc.fx.set_smoke_wind(x, z)
static int M_L_SetSmokeWind(lua_State *const L)
{
    Sparks_SetSmokeWind((XZ_32) {
        .x = luaL_checkinteger(L, 1),
        .z = luaL_checkinteger(L, 2),
    });
    return 0;
}

// trxc.fx.spark_explosion(x, y, z, extras, dynamic, underwater)
static int M_L_SparkExplosion(lua_State *const L)
{
    const XYZ_32 pos = M_ReadPos(L);
    int32_t extras = luaL_checkinteger(L, 4);
    CLAMP(extras, 0, 3);
    int32_t dynamic = luaL_checkinteger(L, 5);
    CLAMP(dynamic, -2, 0);
    Sparks_TriggerExplosionSparks(
        pos, extras, dynamic, lua_toboolean(L, 6) ? 1 : 0, M_CheckRoom(L, pos));
    return 0;
}

// trxc.fx.spark_explosion_smoke(x, y, z, underwater, ending)
static int M_L_SparkExplosionSmoke(lua_State *const L)
{
    const XYZ_32 pos = M_ReadPos(L);
    const bool underwater = lua_toboolean(L, 4);
    const int16_t room_num = M_CheckRoom(L, pos);
    if (lua_toboolean(L, 5)) {
        Sparks_TriggerExplosionSmokeEnd(pos, underwater, room_num);
    } else {
        Sparks_TriggerExplosionSmoke(pos, underwater, room_num);
    }
    return 0;
}

// trxc.fx.spark_explosion_bubble(x, y, z)
static int M_L_SparkExplosionBubble(lua_State *const L)
{
    const XYZ_32 pos = M_ReadPos(L);
    Sparks_TriggerExplosionBubble(pos, M_CheckRoom(L, pos));
    return 0;
}

// trxc.fx.spark_fire_flame(x, y, z, type)
static int M_L_SparkFireFlame(lua_State *const L)
{
    Sparks_TriggerFireFlame(M_ReadPos(L), -1, luaL_checkinteger(L, 4));
    return 0;
}

// trxc.fx.spark_fire_smoke(x, y, z, type)
static int M_L_SparkFireSmoke(lua_State *const L)
{
    Sparks_TriggerFireSmoke(M_ReadPos(L), -1, luaL_checkinteger(L, 4));
    return 0;
}

// trxc.fx.spark_static_flame(x, y, z, size)
static int M_L_SparkStaticFlame(lua_State *const L)
{
    Sparks_TriggerStaticFlame(M_ReadPos(L), luaL_checkinteger(L, 4));
    return 0;
}

// trxc.fx.spark_side_flame(x, y, z, angle, speed, pilot)
static int M_L_SparkSideFlame(lua_State *const L)
{
    Sparks_TriggerSideFlame(
        M_ReadPos(L), luaL_checkinteger(L, 4), luaL_checkinteger(L, 5),
        lua_toboolean(L, 6));
    return 0;
}

// trxc.fx.spark_flamethrower_flame(x, y, z)
static int M_L_SparkFlamethrowerFlame(lua_State *const L)
{
    Sparks_TriggerFlamethrowerHitFlame(M_ReadPos(L));
    return 0;
}

// trxc.fx.spark_flamethrower_smoke(x, y, z, underwater)
static int M_L_SparkFlamethrowerSmoke(lua_State *const L)
{
    Sparks_TriggerFlamethrowerSmoke(M_ReadPos(L), lua_toboolean(L, 4));
    return 0;
}

// trxc.fx.spark_gun_smoke(x, y, z, weapon, shade, initial, vx, vy, vz)
static int M_L_SparkGunSmoke(lua_State *const L)
{
    const XYZ_32 pos = M_ReadPos(L);
    const GAME_VECTOR at = { .pos = pos, .room_num = M_CheckRoom(L, pos) };
    const lua_Integer gun_type = luaL_checkinteger(L, 4);
    if (gun_type <= LGT_UNARMED || !Gun_Registry_IsValidType(gun_type)) {
        return luaL_argerror(L, 4, "not a weapon");
    }
    const LARA_GUN_TYPE weapon = (LARA_GUN_TYPE)gun_type;
    int32_t shade = luaL_checkinteger(L, 5);
    CLAMP(shade, 0, 255);
    const bool initial = lua_toboolean(L, 6);

    if (lua_isnoneornil(L, 7)) {
        Sparks_TriggerGunSmoke(at, initial, weapon, shade);
        return 0;
    }
    const XYZ_32 vel = {
        .x = luaL_checkinteger(L, 7),
        .y = luaL_checkinteger(L, 8),
        .z = luaL_checkinteger(L, 9),
    };
    Sparks_TriggerGunSmokeDirected(at, vel, initial, weapon, shade);
    return 0;
}

// trxc.fx.spark_dart_smoke(x, y, z, vx, vz, hit)
static int M_L_SparkDartSmoke(lua_State *const L)
{
    Sparks_TriggerDartSmoke(
        M_ReadPos(L),
        (XZ_32) {
            .x = luaL_checkinteger(L, 4),
            .z = luaL_checkinteger(L, 5),
        },
        lua_toboolean(L, 6));
    return 0;
}

// trxc.fx.spark_rocket_smoke(x, y, z, spread)
static int M_L_SparkRocketSmoke(lua_State *const L)
{
    const XYZ_32 pos = M_ReadPos(L);
    Sparks_TriggerRocketSmoke(
        pos, luaL_checkinteger(L, 4), M_CheckRoom(L, pos));
    return 0;
}

// trxc.fx.spark_flare(x, y, z, vx, vy, vz, smoke)
static int M_L_SparkFlare(lua_State *const L)
{
    const XYZ_32 pos = M_ReadPos(L);
    Sparks_TriggerFlareSparks(
        pos,
        (XYZ_32) {
            .x = luaL_checkinteger(L, 4),
            .y = luaL_checkinteger(L, 5),
            .z = luaL_checkinteger(L, 6),
        },
        lua_toboolean(L, 7));
    return 0;
}

// trxc.fx.spark_shotgun(x, y, z, vx, vy, vz)
static int M_L_SparkShotgun(lua_State *const L)
{
    const XYZ_32 pos = M_ReadPos(L);
    Sparks_TriggerShotgunSparks(
        pos,
        (XYZ_32) {
            .x = luaL_checkinteger(L, 4),
            .y = luaL_checkinteger(L, 5),
            .z = luaL_checkinteger(L, 6),
        });
    return 0;
}

// trxc.fx.spark_ricochet(x, y, z, angle, count, smoke_only)
static int M_L_SparkRicochet(lua_State *const L)
{
    const XYZ_32 pos = M_ReadPos(L);
    const GAME_VECTOR at = { .pos = pos, .room_num = M_CheckRoom(L, pos) };
    const int32_t angle = luaL_checkinteger(L, 4);
    int32_t count = luaL_checkinteger(L, 5);
    CLAMP(count, 1, 255);
    if (g_TRVersion >= 4) {
        Sparks_TriggerRicochetTR4(at, angle, count, lua_toboolean(L, 6));
    } else {
        Sparks_TriggerRicochetTR3(at, angle, count);
    }
    return 0;
}

// trxc.fx.spark_bubble(x, y, z, size, size_range)
static int M_L_SparkBubble(lua_State *const L)
{
    const XYZ_32 pos = M_ReadPos(L);
    Sparks_TriggerBubble(
        pos.x, pos.y, pos.z, luaL_checkinteger(L, 4), luaL_checkinteger(L, 5),
        NO_EFFECT);
    return 0;
}

// trxc.fx.spark_breath(x, y, z, vx, vy, vz)
static int M_L_SparkBreath(lua_State *const L)
{
    const XYZ_32 pos = M_ReadPos(L);
    Sparks_TriggerBreath(
        pos,
        (XYZ_32) {
            .x = luaL_checkinteger(L, 4),
            .y = luaL_checkinteger(L, 5),
            .z = luaL_checkinteger(L, 6),
        },
        M_CheckRoom(L, pos));
    return 0;
}

// trxc.fx.spark_pickup_aid(x, y, z, vx, vz)
static int M_L_SparkPickupAid(lua_State *const L)
{
    Sparks_TriggerPickupAid(
        M_ReadPos(L),
        (XZ_32) {
            .x = luaL_checkinteger(L, 4),
            .z = luaL_checkinteger(L, 5),
        });
    return 0;
}

// trxc.fx.spark_waterfall_mist(x, y, z, angle)
static int M_L_SparkWaterfallMist(lua_State *const L)
{
    const XYZ_32 pos = M_ReadPos(L);
    Sparks_TriggerWaterfallMist(pos.x, pos.y, pos.z, luaL_checkinteger(L, 4));
    return 0;
}

static const luaL_Reg m_SparkMethods[] = {
    { "kill", M_L_SparkKill },
    { nullptr, nullptr },
};

static const luaL_Reg m_Module[] = {
    { "blood", M_L_Blood },
    { "blood_bath", M_L_BloodBath },
    { "explosion", M_L_Explosion },
    { "fire", M_L_Fire },
    { "footprint", M_L_Footprint },
    { "knockback", M_L_Knockback },
    { "ripple", M_L_Ripple },
    { "small_splash", M_L_SmallSplash },
    { "splash", M_L_Splash },
    { "underwater_blood", M_L_UnderwaterBlood },
    { "wade_splash", M_L_WadeSplash },
    { "emit_light", M_L_EmitLight },
    { "emit_fog", M_L_EmitFog },
    { "fog_bulb_count", M_L_FogBulbCount },
    { "get_fog_bulb", M_L_GetFogBulb },
    { "get_fog_bulb_room", M_L_GetFogBulbRoom },
    { "get_fog_color", M_L_GetFogColor },
    { "set_fog_color", M_L_SetFogColor },
    { "get_smoke_wind", M_L_GetSmokeWind },
    { "set_smoke_wind", M_L_SetSmokeWind },
    { "get_spark", M_L_GetSpark },
    { "get_spark_item", M_L_GetSparkItem },
    { "get_spark_world_pos", M_L_GetSparkWorldPos },
    { "spark_max_count", M_L_SparkMaxCount },
    { "spawn_spark", M_L_SpawnSpark },
    { "spark_breath", M_L_SparkBreath },
    { "spark_bubble", M_L_SparkBubble },
    { "spark_dart_smoke", M_L_SparkDartSmoke },
    { "spark_explosion", M_L_SparkExplosion },
    { "spark_explosion_bubble", M_L_SparkExplosionBubble },
    { "spark_explosion_smoke", M_L_SparkExplosionSmoke },
    { "spark_fire_flame", M_L_SparkFireFlame },
    { "spark_fire_smoke", M_L_SparkFireSmoke },
    { "spark_flamethrower_flame", M_L_SparkFlamethrowerFlame },
    { "spark_flamethrower_smoke", M_L_SparkFlamethrowerSmoke },
    { "spark_flare", M_L_SparkFlare },
    { "spark_gun_smoke", M_L_SparkGunSmoke },
    { "spark_pickup_aid", M_L_SparkPickupAid },
    { "spark_ricochet", M_L_SparkRicochet },
    { "spark_rocket_smoke", M_L_SparkRocketSmoke },
    { "spark_shotgun", M_L_SparkShotgun },
    { "spark_side_flame", M_L_SparkSideFlame },
    { "spark_static_flame", M_L_SparkStaticFlame },
    { "spark_waterfall_mist", M_L_SparkWaterfallMist },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "fx", m_Module);
    LUA_Struct_Register(L, &TYPE_FOG_BULB, nullptr);
    LUA_Struct_Register(L, &TYPE_SPARK, m_SparkMethods);

    LUA_GetModule(L, "fx");
    lua_pushinteger(L, OUTPUT_MAX_PENDING_LIGHTS);
    lua_setfield(L, -2, "MAX_LIGHTS");
    lua_pushinteger(L, OUTPUT_MAX_FOG_BULBS);
    lua_setfield(L, -2, "MAX_FOG");
    lua_pop(L, 1);
}

REGISTER_LUA_CAPI(.create = M_Create)
