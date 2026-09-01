#include <trx/core/vector.h>
#include <trx/game/catalog/manager.h>
#include <trx/game/console.h>
#include <trx/game/items.h>
#include <trx/game/lua/field.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/struct.h>
#include <trx/game/lua/utils.h>
#include <trx/game/objects/common.h>
#include <trx/game/objects/families.h>
#include <trx/game/objects/names.h>
#include <trx/game/objects/setup.h>
#include <trx/game/objects/types.h>

#include <string.h>

// An object is the definition every item of that type is cut from - a wolf's
// radius, not this wolf's. Per-item state lives on the item; see trx.items.

// Defines setup for an object created by a script. The setup is kept across
// level loads because object records are rebuilt for each level.
typedef struct {
    OBJECT_ID object_id;
    int32_t control_ref;
    int32_t initialise_ref;
    int32_t radius;
    int32_t shadow_size;
    bool save_position;
} M_DECLARATION;

static VECTOR *m_Declarations = nullptr;
static lua_State *m_L = nullptr;

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
            L, 1, 0, Catalog_GetCount(CATALOG_OBJECTS) - 1, &object_id)
        || Catalog_IDToKey(CATALOG_OBJECTS, object_id) == nullptr) {
        lua_pushnil(L);
        return 1;
    }
    LUA_Struct_Push(
        L, &TYPE_OBJECT, M_Resolve,
        (TRX_HANDLE) { .id = (OBJECT_ID)object_id });
    return 1;
}

// trxc.objects.swap_mesh(obj1_id, obj2_id, mesh1_num, mesh2_num)

static M_DECLARATION *M_FindDeclaration(const OBJECT_ID object_id)
{
    for (int32_t i = 0; m_Declarations != nullptr && i < m_Declarations->count;
         i++) {
        M_DECLARATION *const decl = Vector_Get(m_Declarations, i);
        if (decl->object_id == object_id) {
            return decl;
        }
    }
    return nullptr;
}

// Calls a script function for an item. Errors are reported to the console.
static void M_CallForItem(
    const int32_t ref, const int16_t item_num, const char *const what)
{
    if (m_L == nullptr || ref == LUA_NOREF) {
        return;
    }
    lua_rawgeti(m_L, LUA_REGISTRYINDEX, ref);
    LUA_PushItem(m_L, item_num);
    if (lua_pcall(m_L, 1, 0, 0) != LUA_OK) {
        Console_ShowError("object %s error: %s", what, lua_tostring(m_L, -1));
        lua_pop(m_L, 1);
    }
}

static void M_Control(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);
    const M_DECLARATION *const decl = M_FindDeclaration(item->object_id);
    if (decl != nullptr) {
        M_CallForItem(decl->control_ref, item_num, "control");
    }
}

static void M_Initialise(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);
    const M_DECLARATION *const decl = M_FindDeclaration(item->object_id);
    if (decl != nullptr) {
        M_CallForItem(decl->initialise_ref, item_num, "initialise");
    }
}

// Applies script setup after the engine sets up its own objects.
static void M_ApplyDeclarations(void)
{
    for (int32_t i = 0; m_Declarations != nullptr && i < m_Declarations->count;
         i++) {
        const M_DECLARATION *const decl = Vector_Get(m_Declarations, i);
        OBJECT *const obj = Object_TryGet(decl->object_id);
        if (obj == nullptr) {
            continue;
        }
        if (decl->control_ref != LUA_NOREF) {
            obj->control_func = M_Control;
        }
        if (decl->initialise_ref != LUA_NOREF) {
            obj->initialise_func = M_Initialise;
        }
        if (decl->radius >= 0) {
            obj->radius = decl->radius;
        }
        if (decl->shadow_size >= 0) {
            obj->shadow_size = decl->shadow_size;
        }
        obj->save_position = decl->save_position;
    }
}

// Takes an optional function from a declaration table.
static int32_t M_TakeFunction(
    lua_State *const L, const int arg, const char *const name)
{
    if (lua_getfield(L, arg, name) == LUA_TNIL) {
        lua_pop(L, 1);
        return LUA_NOREF;
    }
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 1);
        luaL_error(L, "'%s' must be a function", name);
    }
    return luaL_ref(L, LUA_REGISTRYINDEX);
}

static int32_t M_TakeInt(
    lua_State *const L, const int arg, const char *const name)
{
    if (lua_getfield(L, arg, name) == LUA_TNIL) {
        lua_pop(L, 1);
        return -1;
    }
    const int32_t value = (int32_t)luaL_checkinteger(L, -1);
    lua_pop(L, 1);
    return value;
}

// trxc.objects.declare(object_id, spec)
static int M_L_ObjectsDeclare(lua_State *const L)
{
    const OBJECT_ID object_id = LUA_CheckObjectID(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);
    if (M_FindDeclaration(object_id) != nullptr) {
        return luaL_error(
            L, "'%s' is already declared",
            Catalog_IDToKey(CATALOG_OBJECTS, object_id));
    }

    M_DECLARATION decl = {
        .object_id = object_id,
        .control_ref = M_TakeFunction(L, 2, "control"),
        .initialise_ref = M_TakeFunction(L, 2, "initialise"),
        .radius = M_TakeInt(L, 2, "radius"),
        .shadow_size = M_TakeInt(L, 2, "shadow_size"),
    };
    lua_getfield(L, 2, "save_position");
    decl.save_position = lua_toboolean(L, -1);
    lua_pop(L, 1);

    if (m_Declarations == nullptr) {
        m_Declarations = Vector_Create(sizeof(M_DECLARATION));
    }
    Vector_Add(m_Declarations, &decl);
    return 0;
}

// trxc.objects.borrow_content(object_id, source_id) -> bool
static int M_L_ObjectsBorrowContent(lua_State *const L)
{
    const OBJECT_ID object_id = LUA_CheckObjectID(L, 1);
    const OBJECT_ID source_id = LUA_CheckObjectID(L, 2);
    lua_pushboolean(L, IS_OK(Object_BorrowContent(object_id, source_id)));
    return 1;
}

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
    const CATALOG_ID family =
        Catalog_KeyToID(CATALOG_FAMILIES, kind, NO_CATALOG_ID);
    if (family == NO_CATALOG_ID) {
        return luaL_error(L, "unknown object kind '%s'", kind);
    }
    lua_pushboolean(L, ObjectFamily_Has(object_id, family));
    return 1;
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

// Return the family a name states, and raise where no family answers to it.
static CATALOG_ID M_CheckFamily(lua_State *const L, const int arg)
{
    const char *const name = luaL_checkstring(L, arg);
    const CATALOG_ID family =
        Catalog_KeyToID(CATALOG_FAMILIES, name, NO_CATALOG_ID);
    if (family == NO_CATALOG_ID) {
        luaL_error(L, "unknown object kind '%s'", name);
    }
    return family;
}

static int M_L_AddFamily(lua_State *const L)
{
    const LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, &TYPE_OBJECT);
    ObjectFamily_Add((OBJECT_ID)ref->handle.id, M_CheckFamily(L, 2));
    return 0;
}

static int M_L_RemoveFamily(lua_State *const L)
{
    const LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, &TYPE_OBJECT);
    ObjectFamily_Remove((OBJECT_ID)ref->handle.id, M_CheckFamily(L, 2));
    return 0;
}

static const luaL_Reg m_Methods[] = {
    { "get_names", M_L_GetNames },
    { "get_default_names", M_L_GetDefaultNames },
    { "add_family", M_L_AddFamily },
    { "remove_family", M_L_RemoveFamily },
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
    { "borrow_content", M_L_ObjectsBorrowContent },
    { "declare", M_L_ObjectsDeclare },
    { "swap_mesh", M_L_ObjectsSwapMesh },
    { "swap_sprite", M_L_ObjectsSwapSprite },
    { nullptr, nullptr },
};

static void M_Shutdown(void)
{
    for (int32_t i = 0; m_Declarations != nullptr && i < m_Declarations->count;
         i++) {
        const M_DECLARATION *const decl = Vector_Get(m_Declarations, i);
        if (m_L != nullptr) {
            luaL_unref(m_L, LUA_REGISTRYINDEX, decl->control_ref);
            luaL_unref(m_L, LUA_REGISTRYINDEX, decl->initialise_ref);
        }
    }
    if (m_Declarations != nullptr) {
        Vector_Free(m_Declarations);
        m_Declarations = nullptr;
    }
    m_L = nullptr;
}

static void M_Create(lua_State *const L)
{
    m_L = L;
    Object_AddSetupHook(M_ApplyDeclarations);
    LUA_Struct_Register(L, &TYPE_OBJECT, m_Methods);
    LUA_Property_Register(L, &m_Properties);
    LUA_RegisterModule(L, "objects", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create, .shutdown = M_Shutdown)
