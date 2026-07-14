#include <trx/game/lua/utils.h>

#include <trx/game/objects/ids.h>

#include <stdint.h>
#include <string.h>

bool LUA_GetCallerInfo(lua_State *const L, lua_Debug *const ar)
{
    // Level 0 is the bridge. Above it sit the module's binding, a group's
    // __call, and strict mode's wrapper - all of them engine chunks.
    for (int32_t level = 1; lua_getstack(L, level, ar) != 0; level++) {
        if (lua_getinfo(L, "nSl", ar) == 0) {
            return false;
        }
        if (strncmp(
                ar->source, LUA_API_CHUNK_PREFIX,
                sizeof(LUA_API_CHUNK_PREFIX) - 1)
            != 0) {
            return true;
        }
    }
    return false;
}

int32_t LUA_CheckRange(
    lua_State *const L, const int arg, const int32_t count,
    const char *const what)
{
    const lua_Integer value = luaL_checkinteger(L, arg);
    luaL_argcheck(L, value >= 0 && value < count, arg, what);
    return (int32_t)value;
}

OBJECT_ID LUA_CheckObjectID(lua_State *const L, const int arg)
{
    return (OBJECT_ID)LUA_CheckRange(L, arg, O_NUMBER_OF, "unknown object id");
}

void LUA_CheckLogCall(lua_State *const L, LUA_LOG_CALL *const out)
{
    *out = (LUA_LOG_CALL) {
        .level = LUA_CheckRange(L, 1, LOG_LEVEL_ERROR + 1, "unknown log level"),
        .msg = luaL_checkstring(L, 2),
        .src = "?",
        .func = "?",
        .line = 0,
    };
    if (LUA_GetCallerInfo(L, &out->ar)) {
        out->src = out->ar.short_src;
        out->func = out->ar.name != nullptr ? out->ar.name : "?";
        out->line = out->ar.currentline;
    }
}

void LUA_RegisterModule(
    lua_State *const L, const char *const name, const luaL_Reg *const fns)
{
    lua_getglobal(L, "trxc");
    lua_newtable(L);
    luaL_setfuncs(L, fns, 0);
    lua_setfield(L, -2, name);
    lua_pop(L, 1);
}

void LUA_GetModule(lua_State *const L, const char *const name)
{
    lua_getglobal(L, "trxc");
    lua_getfield(L, -1, name);
    lua_remove(L, -2);
}

XYZ_32 LUA_CheckXYZAt(lua_State *const L, const int idx, const int arg)
{
    const int abs_idx = lua_absindex(L, idx);
    luaL_checktype(L, abs_idx, LUA_TTABLE);

    XYZ_32 result = {};
    int32_t *const members[] = { &result.x, &result.y, &result.z };
    static const char *const names[] = { "x", "y", "z" };
    for (int32_t i = 0; i < 3; i++) {
        lua_getfield(L, abs_idx, names[i]);
        int is_integer = 0;
        const lua_Integer value = lua_tointegerx(L, -1, &is_integer);
        if (is_integer == 0 || value < INT32_MIN || value > INT32_MAX) {
            luaL_argerror(
                L, arg, lua_pushfstring(L, "%s must be an integer", names[i]));
        }
        *members[i] = (int32_t)value;
        lua_pop(L, 1);
    }
    return result;
}

XYZ_32 LUA_CheckXYZ(lua_State *const L, const int arg)
{
    return LUA_CheckXYZAt(L, arg, arg);
}

void LUA_PushXYZ(lua_State *const L, const XYZ_32 value)
{
    lua_createtable(L, 0, 3);
    lua_pushinteger(L, value.x);
    lua_setfield(L, -2, "x");
    lua_pushinteger(L, value.y);
    lua_setfield(L, -2, "y");
    lua_pushinteger(L, value.z);
    lua_setfield(L, -2, "z");
}

void LUA_PushPropertyValue(
    lua_State *const L, const OBJECT_PROPERTY_VALUE *const value)
{
    switch (value->type) {
    case OBJECT_PROPERTY_TYPE_INT:
        lua_pushinteger(L, value->as_int);
        break;
    case OBJECT_PROPERTY_TYPE_FLOAT:
        lua_pushnumber(L, value->as_float);
        break;
    case OBJECT_PROPERTY_TYPE_DOUBLE:
        lua_pushnumber(L, value->as_double);
        break;
    case OBJECT_PROPERTY_TYPE_BOOL:
        lua_pushboolean(L, value->as_bool);
        break;
    case OBJECT_PROPERTY_TYPE_XYZ:
        LUA_PushXYZ(L, value->as_xyz);
        break;
    }
}

OBJECT_PROPERTY_VALUE LUA_CheckPropertyValue(lua_State *const L, const int arg)
{
    switch (lua_type(L, arg)) {
    case LUA_TBOOLEAN:
        return (OBJECT_PROPERTY_VALUE) {
            .type = OBJECT_PROPERTY_TYPE_BOOL,
            .as_bool = lua_toboolean(L, arg),
        };

    case LUA_TNUMBER:
        if (lua_isinteger(L, arg)) {
            return (OBJECT_PROPERTY_VALUE) {
                .type = OBJECT_PROPERTY_TYPE_INT,
                .as_int = lua_tointeger(L, arg),
            };
        }
        return (OBJECT_PROPERTY_VALUE) {
            .type = OBJECT_PROPERTY_TYPE_DOUBLE,
            .as_double = lua_tonumber(L, arg),
        };

    case LUA_TTABLE:
        return (OBJECT_PROPERTY_VALUE) {
            .type = OBJECT_PROPERTY_TYPE_XYZ,
            .as_xyz = LUA_CheckXYZ(L, arg),
        };

    default:
        break;
    }

    luaL_error(L, "property value must be a number, boolean or table");
    return (OBJECT_PROPERTY_VALUE) {};
}
