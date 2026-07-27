#include <trx/game/lua/field.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/struct.h>
#include <trx/game/lua/utils.h>
#include <trx/game/objects/names.h>
#include <trx/game/objects/types.h>
#include <trx/game/objects/vars.h>

#include <string.h>

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
    return Object_TryGet((OBJECT_ID)ref->handle.id);
}

// The object property overlay stays a separate namespace: fields address the
// OBJECT struct, properties are what the object declares about itself. The
// bridges themselves are in lua/utils.
static bool M_GetPropertyValue(
    const void *const self, const char *const name, TRX_VALUE *const out)
{
    return ObjectProperty_GetObjectValue(self, name, out);
}

static bool M_SetPropertyValue(
    void *const self, const char *const name, const TRX_VALUE value)
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
    int32_t object_id;
    if (!LUA_CheckBoundedInt(L, 1, O_FIRST, O_NUMBER_OF - 1, &object_id)) {
        lua_pushnil(L);
        return 1;
    }
    LUA_Struct_Push(
        L, &TYPE_OBJECT, M_Resolve,
        (TRX_HANDLE) { .id = (OBJECT_ID)object_id });
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

// The families an object can belong to. A script names one; C knows which array
// it is in.
static const struct {
    const char *name;
    const OBJECT_ID *objects;
} m_Families[] = {
    { "creature", g_CreatureObjects }, //
    { "loyal", g_LoyalObjects }, //
    { "pickup", g_PickupObjects }, //
    { "switch", g_SwitchObjects }, //
    { "receptacle", g_ReceptacleObjects }, //
    { "door", g_DoorObjects }, //
    { "null", g_NullObjects }, //
    { "anim", g_AnimObjects }, //
    { "inventory", g_InvObjects }, //
    { nullptr, nullptr },
};

// trxc.objects.is_type(object_id, kind) -> bool
static int M_L_ObjectsIsType(lua_State *const L)
{
    const OBJECT_ID object_id = luaL_checkinteger(L, 1);
    const char *const kind = luaL_checkstring(L, 2);
    for (int32_t i = 0; m_Families[i].name != nullptr; i++) {
        if (strcmp(kind, m_Families[i].name) == 0) {
            lua_pushboolean(L, Object_IsType(object_id, m_Families[i].objects));
            return 1;
        }
    }
    return luaL_error(L, "unknown object kind '%s'", kind);
}

// The names an object answers to are lists, because it has more than one: a
// large medipack is also a "medipack" and a "big medi". get_names is the
// player's language; get_default_names is the compile-time English fallback a
// lookup takes before any language file is loaded.
static int M_L_GetNames(lua_State *const L)
{
    const LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, &TYPE_OBJECT);
    lua_newtable(L);
    const VECTOR *const names = Object_GetNames((OBJECT_ID)ref->handle.id);
    if (names != nullptr) {
        for (int32_t i = 0; i < names->count; i++) {
            const char *const name = *(char **)Vector_Get(names, i);
            if (name != nullptr) {
                lua_pushstring(L, name);
                lua_seti(L, -2, lua_rawlen(L, -2) + 1);
            }
        }
    }
    return 1;
}

static int M_L_GetDefaultNames(lua_State *const L)
{
    const LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, &TYPE_OBJECT);
    lua_newtable(L);
    const char *const *const names =
        Object_GetDefaultNames((OBJECT_ID)ref->handle.id);
    if (names != nullptr) {
        for (int32_t i = 0; names[i] != nullptr; i++) {
            lua_pushstring(L, names[i]);
            lua_seti(L, -2, i + 1);
        }
    }
    return 1;
}

static const luaL_Reg m_Methods[] = {
    { "get_names", M_L_GetNames },
    { "get_default_names", M_L_GetDefaultNames },
    { nullptr, nullptr },
};

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
    { "is_type", M_L_ObjectsIsType },
    { "swap_mesh", M_L_ObjectsSwapMesh },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_Struct_Register(L, &TYPE_OBJECT, m_Methods);
    LUA_Property_Register(L, &m_Properties);
    LUA_RegisterModule(L, "objects", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
