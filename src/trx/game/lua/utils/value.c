#include <trx/game/lua/utils/value.h>

#include <trx/game/lua/utils/args.h>

#include <lauxlib.h>
#include <stdint.h>

static const char m_ColorCtorKey[] = "trx.color_ctor";

static double M_ColorChannel(const TRX_VALUE *const value, const int32_t idx)
{
    if (value->type == TVT_RGB_F) {
        const float channels[] = {
            value->as_rgb_f.r,
            value->as_rgb_f.g,
            value->as_rgb_f.b,
        };
        return channels[idx] * 255.0;
    }
    const uint8_t channels[] = {
        value->as_rgb.r,
        value->as_rgb.g,
        value->as_rgb.b,
    };
    return channels[idx];
}

static bool M_IsColorType(const TRX_VALUE_TYPE type)
{
    return type == TVT_RGB_888 || type == TVT_RGB_F;
}

static void M_PushColor(
    lua_State *const L, const TRX_VALUE *const value, const int owner_idx,
    const int key_idx)
{
    const int owner = owner_idx != 0 ? lua_absindex(L, owner_idx) : 0;
    const int key = key_idx != 0 ? lua_absindex(L, key_idx) : 0;

    if (lua_getfield(L, LUA_REGISTRYINDEX, m_ColorCtorKey) != LUA_TFUNCTION) {
        // Nothing declared what a color is - a test harness running the
        // bridges without the API modules. The channels are all C has to say.
        lua_pop(L, 1);
        lua_createtable(L, 0, 3);
        static const char *const names[] = { "r", "g", "b" };
        for (int32_t i = 0; i < 3; i++) {
            lua_pushnumber(L, M_ColorChannel(value, i));
            lua_setfield(L, -2, names[i]);
        }
        return;
    }

    for (int32_t i = 0; i < 3; i++) {
        lua_pushnumber(L, M_ColorChannel(value, i));
    }
    if (owner != 0 && key != 0) {
        lua_pushvalue(L, owner);
        lua_pushvalue(L, key);
    } else {
        lua_pushnil(L);
        lua_pushnil(L);
    }
    lua_call(L, 5, 1);
}

static TRX_VALUE M_CheckColor(
    lua_State *const L, const int idx, const TRX_VALUE_TYPE type)
{
    TRX_VALUE value = { .type = type };
    if (lua_isstring(L, idx)) {
        if (!Value_Parse(type, nullptr, lua_tostring(L, idx), &value)) {
            luaL_argerror(L, idx, "not a color");
        }
        return value;
    }

    const int abs_idx = lua_absindex(L, idx);
    luaL_argcheck(
        L, lua_istable(L, abs_idx) || lua_isuserdata(L, abs_idx), idx,
        "expected a color or its hex text");

    double channels[3];
    static const char *const names[] = { "r", "g", "b" };
    for (int32_t i = 0; i < 3; i++) {
        lua_getfield(L, abs_idx, names[i]);
        int is_number = 0;
        channels[i] = lua_tonumberx(L, -1, &is_number);
        if (is_number == 0) {
            luaL_argerror(
                L, idx, lua_pushfstring(L, "%s must be a number", names[i]));
        }
        lua_pop(L, 1);
    }

    if (type == TVT_RGB_F) {
        value.as_rgb_f = (RGB_F) {
            .r = (float)(channels[0] / 255.0),
            .g = (float)(channels[1] / 255.0),
            .b = (float)(channels[2] / 255.0),
        };
        return value;
    }

    for (int32_t i = 0; i < 3; i++) {
        if (channels[i] < 0.0 || channels[i] > 255.0) {
            luaL_argerror(L, idx, "color channel out of range");
        }
    }
    value.as_rgb = (RGB_888) {
        .r = (uint8_t)(channels[0] + 0.5),
        .g = (uint8_t)(channels[1] + 0.5),
        .b = (uint8_t)(channels[2] + 0.5),
    };
    return value;
}

void LUA_SetColorConstructor(lua_State *const L, const int idx)
{
    lua_pushvalue(L, idx);
    lua_setfield(L, LUA_REGISTRYINDEX, m_ColorCtorKey);
}

void LUA_PushValue(lua_State *const L, const TRX_VALUE *const value)
{
    switch (value->type) {
    case TVT_BOOL:
        lua_pushboolean(L, value->as_bool);
        break;

    case TVT_S8:
    case TVT_U8:
    case TVT_S16:
    case TVT_U16:
    case TVT_S32:
    case TVT_U32:
    case TVT_ENUM:
        lua_pushinteger(L, value->as_int);
        break;

    case TVT_FLOAT:
    case TVT_DOUBLE:
        lua_pushnumber(L, value->as_num);
        break;

    case TVT_XYZ_16:
    case TVT_XYZ_32:
        LUA_PushXYZ(L, value->as_xyz);
        break;

    case TVT_RGB_888:
    case TVT_RGB_F:
        M_PushColor(L, value, 0, 0);
        break;

    case TVT_STRING:
    case TVT_DYNAMIC_ENUM:
        if (value->as_str == nullptr) {
            lua_pushnil(L);
        } else {
            lua_pushstring(L, value->as_str);
        }
        break;
    }
}

void LUA_PushMemberValue(
    lua_State *const L, const TRX_VALUE *const value, const int owner_idx,
    const int key_idx)
{
    if (M_IsColorType(value->type)) {
        M_PushColor(L, value, owner_idx, key_idx);
        return;
    }
    LUA_PushValue(L, value);
}

TRX_VALUE LUA_CheckValue(
    lua_State *const L, const int idx, const TRX_VALUE_TYPE type)
{
    TRX_VALUE value = { .type = type };
    switch (type) {
    case TVT_BOOL:
        luaL_checktype(L, idx, LUA_TBOOLEAN);
        value.as_bool = lua_toboolean(L, idx);
        break;

    case TVT_S8:
    case TVT_U8:
    case TVT_S16:
    case TVT_U16:
    case TVT_S32:
    case TVT_U32:
    case TVT_ENUM:
        value.as_int = luaL_checkinteger(L, idx);
        break;

    case TVT_FLOAT:
    case TVT_DOUBLE:
        value.as_num = luaL_checknumber(L, idx);
        break;

    case TVT_XYZ_16:
    case TVT_XYZ_32:
        value.as_xyz = LUA_CheckXYZ(L, idx);
        break;

    case TVT_RGB_888:
    case TVT_RGB_F:
        value = M_CheckColor(L, idx, type);
        break;

    case TVT_STRING:
    case TVT_DYNAMIC_ENUM:
        // nil clears a string field; its setter decides whether that is
        // allowed (e.g. item.name = nil removes the name).
        value.as_str =
            lua_isnoneornil(L, idx) ? nullptr : luaL_checkstring(L, idx);
        break;
    }

    return value;
}
