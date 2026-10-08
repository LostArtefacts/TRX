#include <trx/game/lua/utils/args.h>

#include <trx/game/catalog/manager.h>

#include <lauxlib.h>
#include <stdint.h>

int32_t LUA_CheckRange(
    lua_State *const L, const int arg, const int32_t count,
    const char *const what)
{
    const lua_Integer value = luaL_checkinteger(L, arg);
    luaL_argcheck(L, value >= 0 && value < count, arg, what);
    return (int32_t)value;
}

bool LUA_CheckBoundedInt(
    lua_State *const L, const int arg, const lua_Integer lo,
    const lua_Integer hi, int32_t *const out)
{
    const lua_Integer value = luaL_checkinteger(L, arg);
    if (value < lo || value > hi) {
        return false;
    }
    *out = (int32_t)value;
    return true;
}

OBJECT_ID LUA_CheckObjectID(lua_State *const L, const int arg)
{
    const OBJECT_ID object_id = (OBJECT_ID)LUA_CheckRange(
        L, arg, Catalog_GetCount(CATALOG_OBJECTS), "unknown object id");
    // An anonymous identity answers to no key, so a script has no way to name
    // one and no business holding one.
    luaL_argcheck(
        L, Catalog_IDToKey(CATALOG_OBJECTS, object_id) != nullptr, arg,
        "unknown object id");
    return object_id;
}

void LUA_PushOptIndex(
    lua_State *const L, const int32_t value, const int32_t sentinel)
{
    if (value == sentinel) {
        lua_pushnil(L);
    } else {
        lua_pushinteger(L, value);
    }
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
