#include <trx/game/lua/utils.h>

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
        lua_newtable(L);
        lua_pushinteger(L, value->as_xyz.x);
        lua_setfield(L, -2, "x");
        lua_pushinteger(L, value->as_xyz.y);
        lua_setfield(L, -2, "y");
        lua_pushinteger(L, value->as_xyz.z);
        lua_setfield(L, -2, "z");
        break;
    }
}

OBJECT_PROPERTY_VALUE LUA_CheckPropertyValue(lua_State *const L, const int idx)
{
    switch (lua_type(L, idx)) {
    case LUA_TBOOLEAN:
        return (OBJECT_PROPERTY_VALUE) {
            .type = OBJECT_PROPERTY_TYPE_BOOL,
            .as_bool = lua_toboolean(L, idx),
        };

    case LUA_TNUMBER:
        if (lua_isinteger(L, idx)) {
            return (OBJECT_PROPERTY_VALUE) {
                .type = OBJECT_PROPERTY_TYPE_INT,
                .as_int = lua_tointeger(L, idx),
            };
        }
        return (OBJECT_PROPERTY_VALUE) {
            .type = OBJECT_PROPERTY_TYPE_DOUBLE,
            .as_double = lua_tonumber(L, idx),
        };

    case LUA_TTABLE:
        XYZ_32 vec = {};
        XYZ_32 check = {};

        lua_getfield(L, idx, "x");
        vec.x = lua_tointegerx(L, -1, &check.x);
        lua_pop(L, 1);

        lua_getfield(L, idx, "y");
        vec.y = lua_tointegerx(L, -1, &check.y);
        lua_pop(L, 1);

        lua_getfield(L, idx, "z");
        vec.z = lua_tointegerx(L, -1, &check.z);
        lua_pop(L, 1);

        if (check.x != 0 && check.y != 0 && check.z != 0) {
            return (OBJECT_PROPERTY_VALUE) {
                .type = OBJECT_PROPERTY_TYPE_XYZ,
                .as_xyz = vec,
            };
        }
        break;

    default:
        break;
    }

    luaL_error(L, "property value must be a number, boolean or table");
    return (OBJECT_PROPERTY_VALUE) {};
}
