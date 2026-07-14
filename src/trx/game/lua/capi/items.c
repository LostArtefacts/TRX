#include <trx/game/creature.h>
#include <trx/game/items.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/struct.h>
#include <trx/game/lua/utils.h>
#include <trx/game/objects.h>
#include <trx/game/objects/vars.h>
#include <trx/game/pathing/lot.h>
#include <trx/game/rooms.h>

extern const TYPE_DESC TYPE_ITEM;

// Resolve an item handle. This is where the generation counter earns its keep:
// an index alone would silently rebind to whatever item recycled the slot.
static void *M_Resolve(const LUA_STRUCT_REF *const ref)
{
    if (ref->idx < 0 || ref->idx >= Item_GetTotalCount()) {
        return nullptr;
    }
    ITEM *const item = Item_Get(ref->idx);
    if (item == nullptr || item->gen != ref->gen) {
        return nullptr;
    }
    return item;
}

static void M_PushItem(lua_State *const L, const int16_t idx)
{
    LUA_Struct_Push(L, &TYPE_ITEM, M_Resolve, idx, Item_Get(idx)->gen);
}

// The object property overlay stays a separate namespace: fields address the
// ITEM struct, properties are the object's declared defaults plus per-item
// sparse overrides.
static int M_L_GetProperty(lua_State *const L)
{
    LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, &TYPE_ITEM);
    const ITEM *const item = LUA_Struct_Deref(L, ref);
    OBJECT_PROPERTY_VALUE value = {};
    if (!ObjectProperty_GetItemValue(item, luaL_checkstring(L, 2), &value)) {
        lua_pushnil(L);
        return 1;
    }
    LUA_PushPropertyValue(L, &value);
    return 1;
}

static int M_L_SetProperty(lua_State *const L)
{
    LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, &TYPE_ITEM);
    ITEM *const item = LUA_Struct_Deref(L, ref);
    const char *const name = luaL_checkstring(L, 2);
    const OBJECT_PROPERTY_VALUE value = LUA_CheckPropertyValue(L, 3);
    if (!ObjectProperty_SetItemValueRaw(item, name, value)) {
        return luaL_error(L, "unknown item property '%s'", name);
    }
    return 0;
}

static int M_L_GetPropertyNames(lua_State *const L)
{
    LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, &TYPE_ITEM);
    const ITEM *const item = LUA_Struct_Deref(L, ref);
    lua_newtable(L);
    for (int32_t i = 0; i < ObjectProperty_GetItemNameCount(item); i++) {
        lua_pushinteger(L, i + 1);
        lua_pushstring(L, ObjectProperty_GetItemName(item, i));
        lua_settable(L, -3);
    }
    return 1;
}

static int M_L_Kill(lua_State *const L)
{
    LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, &TYPE_ITEM);
    LUA_Struct_Deref(L, ref);
    Item_Kill(ref->idx);
    return 0;
}

static int M_L_Activate(lua_State *const L)
{
    LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, &TYPE_ITEM);
    ITEM *const item = LUA_Struct_Deref(L, ref);
    Item_AddActive(ref->idx);
    item->status = IS_ACTIVE;
    return 0;
}

static int M_L_IsValid(lua_State *const L)
{
    LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, &TYPE_ITEM);
    lua_pushboolean(L, ref->resolve(ref) != nullptr);
    return 1;
}

// trxc.items.count() -> int
static int M_L_Count(lua_State *const L)
{
    lua_pushinteger(L, Item_GetTotalCount());
    return 1;
}

// trxc.items.get(index | name) -> Item or nil
static int M_L_Get(lua_State *const L)
{
    int32_t idx = -1;
    if (lua_type(L, 1) == LUA_TNUMBER) {
        // Read wide and range-test before narrowing: an index past int32_t's
        // width would wrap into range and name an item the script did not ask
        // for.
        const lua_Integer num = luaL_checkinteger(L, 1);
        if (num < 1 || num > Item_GetTotalCount()) {
            lua_pushnil(L);
            return 1;
        }
        idx = (int32_t)(num - 1);
    } else {
        const ITEM *const item = Item_GetByName(luaL_checkstring(L, 1));
        idx = item != nullptr ? Item_GetIndex(item) : -1;
    }
    if (idx < 0 || idx >= Item_GetTotalCount()) {
        lua_pushnil(L);
        return 1;
    }
    M_PushItem(L, idx);
    return 1;
}

// trxc.items.spawn(object_id, {x,y,z}, angle_y) -> Item or nil
static int M_L_Spawn(lua_State *const L)
{
    // Object_Get asserts on an id outside the table.
    const OBJECT_ID object_id = LUA_CheckObjectID(L, 1);
    const OBJECT *const obj = Object_Get(object_id);
    if (!obj->loaded) {
        return luaL_error(L, "object %d is not loaded", object_id);
    }

    luaL_checktype(L, 2, LUA_TTABLE);
    XYZ_32 pos = {};
    lua_getfield(L, 2, "x");
    pos.x = luaL_checkinteger(L, -1);
    lua_getfield(L, 2, "y");
    pos.y = luaL_checkinteger(L, -1);
    lua_getfield(L, 2, "z");
    pos.z = luaL_checkinteger(L, -1);
    lua_pop(L, 3);

    const int16_t room_num = Room_GetIndexFromPos(pos);
    if (room_num == NO_ROOM) {
        return luaL_error(L, "position is outside the level");
    }

    // Read every argument that can raise before taking a slot: luaL_optinteger
    // longjmps on a non-numeric argument, and doing it after Item_Create would
    // leave a half-initialised item occupying the pool.
    const lua_Integer rot_y = luaL_optinteger(L, 3, 0);

    const int16_t idx = Item_Create();
    if (idx == NO_ITEM) {
        lua_pushnil(L);
        return 1;
    }

    ITEM *const item = Item_Get(idx);
    item->object_id = object_id;
    item->pos = pos;
    item->rot.y = rot_y;
    item->room_num = room_num;
    item->shade.value_1 = -1;
    Item_Initialise(idx);

    // opts.activate: bring the item to life the way the spawn cheat does -
    // creatures additionally need their AI enabled or they stand inert.
    if (lua_istable(L, 4)) {
        lua_getfield(L, 4, "activate");
        const bool activate = lua_toboolean(L, -1);
        lua_pop(L, 1);
        if (activate) {
            Item_AddActive(idx);
            if (Object_IsType(object_id, g_CreatureObjects)
                || Object_IsType(object_id, g_LoyalObjects)
                || obj->intelligent) {
                item->status = IS_ACTIVE;
                LOT_EnableBaddieAI(idx, true);
            }
        }
    }

    M_PushItem(L, idx);
    return 1;
}

// item:distance_to({x,y,z}) -> integer
static int M_L_DistanceTo(lua_State *const L)
{
    LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, &TYPE_ITEM);
    const ITEM *const item = LUA_Struct_Deref(L, ref);
    luaL_checktype(L, 2, LUA_TTABLE);
    XYZ_32 target = {};
    lua_getfield(L, 2, "x");
    target.x = luaL_checkinteger(L, -1);
    lua_getfield(L, 2, "y");
    target.y = luaL_checkinteger(L, -1);
    lua_getfield(L, 2, "z");
    target.z = luaL_checkinteger(L, -1);
    lua_pop(L, 3);
    lua_pushinteger(L, Item_GetDistance(item, target));
    return 1;
}

// item:explode()
//
// Runs the object's death handling with an explosion. A primitive: the `kill`
// cheat is one composition of it, and lives in Lua.
static int M_L_Explode(lua_State *const L)
{
    LUA_STRUCT_REF *const ref = LUA_Struct_CheckRef(L, 1, &TYPE_ITEM);
    LUA_Struct_Deref(L, ref);
    Creature_Die(ref->idx, true);
    return 0;
}

static const luaL_Reg m_Methods[] = {
    { "distance_to", M_L_DistanceTo },
    { "explode", M_L_Explode },
    { "kill", M_L_Kill },
    { "activate", M_L_Activate },
    { "is_valid", M_L_IsValid },
    { "get_property", M_L_GetProperty },
    { "set_property", M_L_SetProperty },
    { "get_property_names", M_L_GetPropertyNames },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_Struct_Register(L, &TYPE_ITEM, m_Methods);

    lua_getglobal(L, "trxc");
    lua_newtable(L);
    lua_pushcfunction(L, M_L_Count);
    lua_setfield(L, -2, "count");
    lua_pushcfunction(L, M_L_Get);
    lua_setfield(L, -2, "get");
    lua_pushcfunction(L, M_L_Spawn);
    lua_setfield(L, -2, "spawn");
    lua_setfield(L, -2, "items");
    lua_pop(L, 1);
}

REGISTER_LUA_CAPI(.create = M_Create)
