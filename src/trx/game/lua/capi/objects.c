#include <trx/game/lua/field.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/struct.h>
#include <trx/game/lua/utils.h>
#include <trx/game/objects/types.h>

// An object is the definition every item of that type is cut from - a wolf's
// radius, not this wolf's. Per-item state lives on the item; see trx.items.

// clang-format off
static const FIELD_DESC m_Fields[] = {
    // what the level actually has
    FIELD_RO(OBJECT, loaded),
    FIELD_RO(OBJECT, intelligent),
    FIELD_RO(OBJECT, mesh_count),
    FIELD_RO(OBJECT, anim_count),

    // the numbers that decide how it behaves
    FIELD(OBJECT, radius),
    FIELD(OBJECT, shadow_size),
    FIELD(OBJECT, smartness),
    FIELD(OBJECT, pivot_length),
    FIELD(OBJECT, semi_transparent),

    // Deliberately absent: every _func pointer, the mesh and animation indices,
    // and the frame data. They are how the engine drives an object, and a script
    // moving them would point it at the wrong meshes.
};
// clang-format on

TYPE_DEFINE(OBJECT, m_Fields)

// An object is addressed by its id, and an id is valid for the whole session -
// there is no pool to recycle and nothing to go stale. An object the level did
// not load still exists as a definition; `loaded` is how a script tells.
static void *M_Resolve(const LUA_STRUCT_REF *const ref)
{
    return Object_TryGet((OBJECT_ID)ref->idx);
}

// The object property overlay stays a separate namespace: fields address the
// OBJECT struct, properties are what the object declares about itself. The
// bridges themselves are in lua/utils.
static bool M_GetPropertyValue(
    const void *const self, const char *const name,
    OBJECT_PROPERTY_VALUE *const out)
{
    return ObjectProperty_GetObjectValue(self, name, out);
}

static bool M_SetPropertyValue(
    void *const self, const char *const name, const OBJECT_PROPERTY_VALUE value)
{
    return ObjectProperty_SetObjectValueRaw(self, name, value);
}

static int32_t M_GetPropertyCount(const void *const self)
{
    return ObjectProperty_GetObjectNameCount(self);
}

static const char *M_GetPropertyName(const void *const self, const int32_t idx)
{
    return ObjectProperty_GetObjectName(self, idx);
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
    if (arg_count != 4) {
        return luaL_error(L, "swap_mesh takes both mesh numbers, or neither");
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

static const LUA_PROPERTY_DESC m_Properties = {
    .type = &TYPE_OBJECT,
    .what = "object",
    .get = M_GetPropertyValue,
    .set = M_SetPropertyValue,
    .name_count = M_GetPropertyCount,
    .name_at = M_GetPropertyName,
};

static const luaL_Reg m_Module[] = {
    { "get", M_L_ObjectsGet },
    { "swap_mesh", M_L_ObjectsSwapMesh },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_Struct_Register(L, &TYPE_OBJECT, nullptr);
    LUA_Property_Register(L, &m_Properties);
    LUA_RegisterModule(L, "objects", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
