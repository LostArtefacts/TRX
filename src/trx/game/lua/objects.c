#include <trx/game/lua/utils.h>

static void M_PushPropertyValue(
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

// trxc.objects.swap_mesh(obj1_id, obj2_id, mesh1_num, mesh2_num)
static int M_L_ObjectsSwapMesh(lua_State *const L)
{
    const int32_t arg_count = lua_gettop(L);
    const OBJECT_ID obj1_id = luaL_checkinteger(L, 1);
    const OBJECT_ID obj2_id = luaL_checkinteger(L, 2);
    if (arg_count == 2) {
        Object_SwapAllMeshes(obj1_id, obj2_id);
    } else {
        const int32_t mesh1_num = luaL_checkinteger(L, 3);
        const int32_t mesh2_num = luaL_checkinteger(L, 4);
        Object_SwapMeshEx(obj1_id, obj2_id, mesh1_num, mesh2_num);
    }
    return 0;
}

// trxc.objects.get_property(object_id, name) → typed value or nil
static int M_L_ObjectsGetProperty(lua_State *const L)
{
    const OBJECT_ID object_id = luaL_checkinteger(L, 1);
    const char *const name = luaL_checkstring(L, 2);
    OBJECT_PROPERTY_VALUE value = {};
    if (!ObjectProperty_GetObjectValue(
            Object_TryGet(object_id), name, &value)) {
        lua_pushnil(L);
        return 1;
    }
    M_PushPropertyValue(L, &value);
    return 1;
}

// trxc.objects.set_property(object_id, name, value)
static int M_L_ObjectsSetProperty(lua_State *const L)
{
    const OBJECT_ID object_id = luaL_checkinteger(L, 1);
    const char *const name = luaL_checkstring(L, 2);
    OBJECT *const obj = Object_TryGet(object_id);
    if (obj == nullptr) {
        return luaL_error(L, "invalid object id %d", object_id);
    }
    const OBJECT_PROPERTY_VALUE value = LUA_CheckPropertyValue(L, 3);
    if (!ObjectProperty_SetObjectValueRaw(obj, name, value)) {
        return luaL_error(L, "unknown object property '%s'", name);
    }
    return 0;
}

// trxc.objects.get_property_names(object_id) → table
static int M_L_ObjectsGetPropertyNames(lua_State *const L)
{
    const OBJECT_ID object_id = luaL_checkinteger(L, 1);
    const OBJECT *const obj = Object_TryGet(object_id);
    lua_newtable(L);
    for (int32_t i = 0; i < ObjectProperty_GetObjectNameCount(obj); i++) {
        lua_pushinteger(L, i + 1);
        lua_pushstring(L, ObjectProperty_GetObjectName(obj, i));
        lua_settable(L, -3);
    }
    return 1;
}

void LUA_CreateObjects(lua_State *const L)
{
    lua_getglobal(L, "trxc");
    lua_newtable(L);
    lua_pushcfunction(L, M_L_ObjectsSwapMesh);
    lua_setfield(L, -2, "swap_mesh");
    lua_pushcfunction(L, M_L_ObjectsGetProperty);
    lua_setfield(L, -2, "get_property");
    lua_pushcfunction(L, M_L_ObjectsSetProperty);
    lua_setfield(L, -2, "set_property");
    lua_pushcfunction(L, M_L_ObjectsGetPropertyNames);
    lua_setfield(L, -2, "get_property_names");
    lua_setfield(L, -2, "objects");
    lua_pop(L, 1);
}
