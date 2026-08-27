#include <trx/game/catalog/manager.h>
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
    // what the level holds
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

// What a pickup is, as pickups.def already divides them. These are a scripting
// concept and nothing in the engine asks for them, so they stay here rather
// than joining the global object arrays.
//
// The engine's own g_QuestObjects is left alone: the carrier and the pickup
// routine read it, and the scion is not theirs to gain.
static const OBJECT_ID m_SupplyObjects[] = {
#define X_PICKUP_SUPPLY(item, option) item,
#define X_PICKUP_SUPPLY_VARIANT(item, option) item,
#include <trx/game/objects/pickups.def>
#undef X_PICKUP_SUPPLY_VARIANT
#undef X_PICKUP_SUPPLY
    NO_OBJECT,
};

// What Lara carries and uses: the crowbar, the lasersight, the binoculars, the
// waterskins and the leadbar. A level's story may turn on one of them and one
// may serve across a whole game; what they share is being named for themselves
// rather than filling a numbered slot.
static const OBJECT_ID m_ToolObjects[] = {
#define X_PICKUP_MISC(item, option) item,
#include <trx/game/objects/pickups.def>
#undef X_PICKUP_MISC
    NO_OBJECT,
};

static const OBJECT_ID m_KeyObjects[] = {
#define X_PICKUP_KEY(n) O_KEY_ITEM_##n,
#define X_PICKUP_KEY_COMBO(n, c) O_KEY_ITEM_##n##_COMBO_##c,
#include <trx/game/objects/pickups.def>
#undef X_PICKUP_KEY_COMBO
#undef X_PICKUP_KEY
    NO_OBJECT,
};

static const OBJECT_ID m_PuzzleObjects[] = {
#define X_PICKUP_PUZZLE(n) O_PUZZLE_ITEM_##n,
#define X_PICKUP_PUZZLE_COMBO(n, c) O_PUZZLE_ITEM_##n##_COMBO_##c,
#include <trx/game/objects/pickups.def>
#undef X_PICKUP_PUZZLE_COMBO
#undef X_PICKUP_PUZZLE
    NO_OBJECT,
};

// The scion is here rather than in a family of its own: it is what TR1 sends
// Lara to fetch, which is what a quest item is.
static const OBJECT_ID m_QuestObjects[] = {
#define X_PICKUP_QUEST(n) O_QUEST_ITEM_##n,
#define X_PICKUP_SPECIAL(item, option) item,
#include <trx/game/objects/pickups.def>
#undef X_PICKUP_SPECIAL
#undef X_PICKUP_QUEST
    NO_OBJECT,
};

static const OBJECT_ID m_ExamineObjects[] = {
#define X_PICKUP_EXAMINE(n) O_EXAMINE_ITEM_##n,
#include <trx/game/objects/pickups.def>
#undef X_PICKUP_EXAMINE
    NO_OBJECT,
};

static const OBJECT_ID m_CollectibleObjects[] = {
#define X_PICKUP_PICKUP(n) O_PICKUP_ITEM_##n,
#define X_PICKUP_PICKUP_COMBO(n, c) O_PICKUP_ITEM_##n##_COMBO_##c,
#include <trx/game/objects/pickups.def>
#undef X_PICKUP_PICKUP_COMBO
#undef X_PICKUP_PICKUP
    NO_OBJECT,
};

// The families an object can belong to. A script names one; C knows which array
// it is in.
static const struct {
    const char *name;
    const OBJECT_ID *objects;
} m_Families[] = {
    { "creature", g_CreatureObjects },
    { "boss", g_BossObjects },
    { "loyal", g_LoyalObjects },
    { "pickup", g_PickupObjects },
    { "gun", g_GunObjects },
    { "ammo", g_GunAmmoObjects },
    { "supply", m_SupplyObjects },
    { "tool", m_ToolObjects },
    { "key", m_KeyObjects },
    { "puzzle", m_PuzzleObjects },
    { "quest", m_QuestObjects },
    { "examine", m_ExamineObjects },
    { "collectible", m_CollectibleObjects },
    { "secret", g_SecretObjects },
    { "switch", g_SwitchObjects },
    { "receptacle", g_ReceptacleObjects },
    { "pushable", g_MovableBlockObjects },
    { "door", g_DoorObjects },
    { "null", g_NullObjects },
    { "anim", g_AnimObjects },
    { "inventory", g_InvObjects },
    { nullptr, nullptr },
};

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

static const char *M_SetPropertyValue(
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
    if (!LUA_CheckBoundedInt(
            L, 1, O_FIRST, Catalog_GetCount(CATALOG_OBJECTS) - 1, &object_id)
        || Catalog_GetKey(CATALOG_OBJECTS, object_id) == nullptr) {
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

// trxc.objects.swap_sprite(obj1_id, obj2_id)
static int M_L_ObjectsSwapSprite(lua_State *const L)
{
    const OBJECT_ID obj1_id = LUA_CheckObjectID(L, 1);
    const OBJECT_ID obj2_id = LUA_CheckObjectID(L, 2);
    const OBJECT *const obj1 = Object_Get(obj1_id);
    const OBJECT *const obj2 = Object_Get(obj2_id);
    if (!obj1->loaded || !obj2->loaded) {
        return 0;
    }

    // A modelled object holds meshes in the slots this writes, so moving a
    // sprite into one would draw it from mesh data.
    if (obj1->mesh_count >= 0 || obj2->mesh_count >= 0) {
        return luaL_error(L, "both objects must be drawn as sprites");
    }

    Object_SwapSprite(obj1_id, obj2_id);
    return 0;
}

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

// Push the primary name followed by aliases.
static int M_L_PushNames(
    lua_State *const L, const char *const name, const char *const aliases)
{
    lua_newtable(L);
    int32_t count = 0;
    if (name != nullptr) {
        lua_pushstring(L, name);
        lua_seti(L, -2, ++count);
    }
    for (const char *cur = aliases; cur != nullptr && *cur != '\0';) {
        const char *const sep = strchr(cur, '|');
        if (sep == nullptr) {
            lua_pushstring(L, cur);
            cur = nullptr;
        } else {
            lua_pushlstring(L, cur, sep - cur);
            cur = sep + 1;
        }
        lua_seti(L, -2, ++count);
    }
    return 1;
}

static int M_L_GetNames(lua_State *const L)
{
    const LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, &TYPE_OBJECT);
    const OBJECT_ID obj_id = (OBJECT_ID)ref->handle.id;
    return M_L_PushNames(L, Object_GetName(obj_id), Object_GetAliases(obj_id));
}

static int M_L_GetDefaultNames(lua_State *const L)
{
    const LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, &TYPE_OBJECT);
    const OBJECT_ID obj_id = (OBJECT_ID)ref->handle.id;
    return M_L_PushNames(
        L, Object_GetDefaultName(obj_id), Object_GetDefaultAliases(obj_id));
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
    { "swap_sprite", M_L_ObjectsSwapSprite },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_Struct_Register(L, &TYPE_OBJECT, m_Methods);
    LUA_Property_Register(L, &m_Properties);
    LUA_RegisterModule(L, "objects", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
