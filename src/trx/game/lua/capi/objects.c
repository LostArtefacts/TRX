#include <trx/game/lua/registry.h>
#include <trx/game/lua/struct.h>
#include <trx/game/lua/utils.h>

extern const TYPE_DESC TYPE_OBJECT;

// An object is addressed by its id, and an id is valid for the whole session -
// there is no pool to recycle and nothing to go stale. An object the level did
// not load still exists as a definition; `loaded` is how a script tells.
static void *M_Resolve(const LUA_STRUCT_REF *const ref)
{
    return Object_TryGet((OBJECT_ID)ref->idx);
}

// trxc.objects.get(object_id) -> OBJECT handle or nil
static int M_L_ObjectsGet(lua_State *const L)
{
    // Read wide and range-test before narrowing: an id past OBJECT_ID's width
    // would wrap into the table and name an object the script did not ask for.
    const lua_Integer object_id = luaL_checkinteger(L, 1);
    if (object_id < O_FIRST || object_id >= O_NUMBER_OF) {
        lua_pushnil(L);
        return 1;
    }
    LUA_Struct_Push(L, &TYPE_OBJECT, M_Resolve, (OBJECT_ID)object_id, 0);
    return 1;
}

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
    const OBJECT_ID obj1_id = LUA_CheckObjectID(L, 1);
    const OBJECT_ID obj2_id = LUA_CheckObjectID(L, 2);
    if (arg_count == 2) {
        Object_SwapAllMeshes(obj1_id, obj2_id);
        return 0;
    }

    // An object that did not load has no meshes, and no count to measure a mesh
    // number against.
    const OBJECT *const obj1 = Object_Get(obj1_id);
    const OBJECT *const obj2 = Object_Get(obj2_id);
    if (!obj1->loaded || !obj2->loaded) {
        return 0;
    }

    // Object_SwapMeshEx swaps the two pointers at an offset it does not check.
    const int32_t mesh1_num =
        LUA_CheckRange(L, 3, obj1->mesh_count, "no such mesh on this object");
    const int32_t mesh2_num =
        LUA_CheckRange(L, 4, obj2->mesh_count, "no such mesh on this object");
    Object_SwapMeshEx(obj1_id, obj2_id, mesh1_num, mesh2_num);
    return 0;
}

// The object property overlay stays a separate namespace: fields address the
// OBJECT struct, properties are what the object declares about itself.
static int M_L_GetProperty(lua_State *const L)
{
    LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, &TYPE_OBJECT);
    const OBJECT *const obj = LUA_Struct_Deref(L, ref);
    OBJECT_PROPERTY_VALUE value = {};
    if (!ObjectProperty_GetObjectValue(obj, luaL_checkstring(L, 2), &value)) {
        lua_pushnil(L);
        return 1;
    }
    M_PushPropertyValue(L, &value);
    return 1;
}

static int M_L_SetProperty(lua_State *const L)
{
    LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, &TYPE_OBJECT);
    OBJECT *const obj = LUA_Struct_Deref(L, ref);
    const char *const name = luaL_checkstring(L, 2);
    const OBJECT_PROPERTY_VALUE value = LUA_CheckPropertyValue(L, 3);
    if (!ObjectProperty_SetObjectValueRaw(obj, name, value)) {
        return luaL_error(L, "unknown object property '%s'", name);
    }
    return 0;
}

static int M_L_GetPropertyNames(lua_State *const L)
{
    LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, &TYPE_OBJECT);
    const OBJECT *const obj = LUA_Struct_Deref(L, ref);
    lua_newtable(L);
    for (int32_t i = 0; i < ObjectProperty_GetObjectNameCount(obj); i++) {
        lua_pushinteger(L, i + 1);
        lua_pushstring(L, ObjectProperty_GetObjectName(obj, i));
        lua_settable(L, -3);
    }
    return 1;
}

static const luaL_Reg m_Methods[] = {
    { "get_property", M_L_GetProperty },
    { "set_property", M_L_SetProperty },
    { "get_property_names", M_L_GetPropertyNames },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_Struct_Register(L, &TYPE_OBJECT, m_Methods);

    lua_getglobal(L, "trxc");
    lua_newtable(L);
    lua_pushcfunction(L, M_L_ObjectsGet);
    lua_setfield(L, -2, "get");
    lua_pushcfunction(L, M_L_ObjectsSwapMesh);
    lua_setfield(L, -2, "swap_mesh");
    lua_setfield(L, -2, "objects");
    lua_pop(L, 1);
}

REGISTER_LUA_CAPI(.create = M_Create)
