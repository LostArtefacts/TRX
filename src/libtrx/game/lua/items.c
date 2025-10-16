#include "game/items.h"
#include "game/lua/common.h"
#include "game/lua/polyfill.h"
#include "game/rooms.h"
#include "utils.h"

#include <lauxlib.h>

// Registry-based getters/setters for TRX.Items.ITEM
// Define C getters and setters for each field and register in init

static int M_L_ItemGet_pos(lua_State *const L)
{
    const ITEM *const item = *(ITEM **)luaL_checkudata(L, 1, "TRX.Items.ITEM");
    lua_newtable(L);
    lua_pushinteger(L, item->pos.x);
    lua_setfield(L, -2, "x");
    lua_pushinteger(L, item->pos.y);
    lua_setfield(L, -2, "y");
    lua_pushinteger(L, item->pos.z);
    lua_setfield(L, -2, "z");
    return 1;
}

static int M_L_ItemGet_rot(lua_State *const L)
{
    const ITEM *const item = *(ITEM **)luaL_checkudata(L, 1, "TRX.Items.ITEM");
    lua_newtable(L);
    lua_pushinteger(L, item->rot.x);
    lua_setfield(L, -2, "x");
    lua_pushinteger(L, item->rot.y);
    lua_setfield(L, -2, "y");
    lua_pushinteger(L, item->rot.z);
    lua_setfield(L, -2, "z");
    return 1;
}

static int M_L_ItemGet_room(lua_State *const L)
{
    const ITEM *const item = *(ITEM **)luaL_checkudata(L, 1, "TRX.Items.ITEM");
    lua_pushinteger(L, item->room_num);
    return 1;
}

static int M_L_ItemGet_status(lua_State *const L)
{
    const ITEM *const item = *(ITEM **)luaL_checkudata(L, 1, "TRX.Items.ITEM");
    lua_pushinteger(L, (int)item->status);
    return 1;
}

static int M_L_ItemGet_object_id(lua_State *const L)
{
    const ITEM *const item = *(ITEM **)luaL_checkudata(L, 1, "TRX.Items.ITEM");
    lua_pushinteger(L, item->object_id);
    return 1;
}

static int M_L_ItemGet_hit_points(lua_State *const L)
{
    const ITEM *const item = *(ITEM **)luaL_checkudata(L, 1, "TRX.Items.ITEM");
    lua_pushinteger(L, item->hit_points);
    return 1;
}

static int M_L_ItemGet_max_hit_points(lua_State *const L)
{
    const ITEM *const item = *(ITEM **)luaL_checkudata(L, 1, "TRX.Items.ITEM");
    lua_pushinteger(L, item->max_hit_points);
    return 1;
}

static int M_L_ItemGet_name(lua_State *const L)
{
    const ITEM *const item = *(ITEM **)luaL_checkudata(L, 1, "TRX.Items.ITEM");
    if (item->name != nullptr) {
        lua_pushstring(L, item->name);
    } else {
        lua_pushnil(L);
    }
    return 1;
}

static int M_L_ItemSet_pos(lua_State *const L)
{
    ITEM *const item = *(ITEM **)luaL_checkudata(L, 1, "TRX.Items.ITEM");
    luaL_checktype(L, 2, LUA_TTABLE);
    lua_getfield(L, 2, "x");
    item->pos.x = luaL_checkinteger(L, -1);
    lua_pop(L, 1);
    lua_getfield(L, 2, "y");
    item->pos.y = luaL_checkinteger(L, -1);
    lua_pop(L, 1);
    lua_getfield(L, 2, "z");
    item->pos.z = luaL_checkinteger(L, -1);
    lua_pop(L, 1);
    item->room_num =
        Room_GetIndexFromPos(item->pos.x, item->pos.y, item->pos.z);
    return 0;
}

static int M_L_ItemSet_hit_points(lua_State *const L)
{
    ITEM *const item = *(ITEM **)luaL_checkudata(L, 1, "TRX.Items.ITEM");
    item->hit_points = luaL_checkinteger(L, 2);
    item->max_hit_points = MAX(item->hit_points, item->max_hit_points);
    return 0;
}

static int M_L_ItemSet_max_hit_points(lua_State *const L)
{
    ITEM *const item = *(ITEM **)luaL_checkudata(L, 1, "TRX.Items.ITEM");
    item->max_hit_points = luaL_checkinteger(L, 2);
    return 0;
}

static int M_L_ItemSet_rot(lua_State *const L)
{
    ITEM *const item = *(ITEM **)luaL_checkudata(L, 1, "TRX.Items.ITEM");
    luaL_checktype(L, 2, LUA_TTABLE);
    lua_getfield(L, 2, "x");
    item->rot.x = luaL_checkinteger(L, -1);
    lua_pop(L, 1);
    lua_getfield(L, 2, "y");
    item->rot.y = luaL_checkinteger(L, -1);
    lua_pop(L, 1);
    lua_getfield(L, 2, "z");
    item->rot.z = luaL_checkinteger(L, -1);
    lua_pop(L, 1);
    return 0;
}

static int M_L_ItemSet_name(lua_State *const L)
{
    ITEM *const item = *(ITEM **)luaL_checkudata(L, 1, "TRX.Items.ITEM");
    const char *const new_name = luaL_checkstring(L, 2);
    if (!Item_SetName(Item_GetIndex(item), new_name)) {
        return luaL_error(L, "item name '%s' already in use", new_name);
    }
    return 0;
}

static const luaL_Reg m_ItemGetters[] = {
    { "pos", M_L_ItemGet_pos },
    { "rot", M_L_ItemGet_rot },
    { "room", M_L_ItemGet_room },
    { "status", M_L_ItemGet_status },
    { "object_id", M_L_ItemGet_object_id },
    { "hit_points", M_L_ItemGet_hit_points },
    { "max_hit_points", M_L_ItemGet_max_hit_points },
    { "name", M_L_ItemGet_name },
    { nullptr, nullptr }
};

static const luaL_Reg m_ItemSetters[] = {
    { "pos", M_L_ItemSet_pos },
    { "hit_points", M_L_ItemSet_hit_points },
    { "max_hit_points", M_L_ItemSet_max_hit_points },
    { "rot", M_L_ItemSet_rot },
    { "name", M_L_ItemSet_name },
    { nullptr, nullptr }
};

// Lua metamethod: __index for TRX.Items.ITEM read fields
static int M_L_ItemIndex(lua_State *const L)
{
    const char *const key = luaL_checkstring(L, 2);
    luaL_getsubtable(L, LUA_REGISTRYINDEX, "TRX.ItemGetters");
    lua_pushstring(L, key);
    lua_rawget(L, -2);
    lua_remove(L, -2);
    if (lua_isfunction(L, -1)) {
        lua_pushvalue(L, 1);
        lua_call(L, 1, 1);
        return 1;
    }
    lua_pop(L, 1);
    lua_pushnil(L);
    return 1;
}

// Lua metamethod: __newindex for writable fields
static int M_L_ItemNewIndex(lua_State *const L)
{
    const char *const key = luaL_checkstring(L, 2);
    luaL_getsubtable(L, LUA_REGISTRYINDEX, "TRX.ItemSetters");
    lua_pushstring(L, key);
    lua_rawget(L, -2);
    lua_remove(L, -2);
    if (lua_isfunction(L, -1)) {
        lua_pushvalue(L, 1);
        lua_pushvalue(L, 3);
        lua_call(L, 2, 0);
        return 0;
    }
    return luaL_error(L, "Cannot set field '%s' on TRX.Items.ITEM", key);
}

// item_count = TRX.Items.Count()
static int M_L_ItemsCount(lua_State *const L)
{
    lua_pushinteger(L, Item_GetTotalCount());
    return 1;
}

// item = TRX.Items.Get(idx) or TRX.Items.Get(name)
static int M_L_ItemsGet(lua_State *const L)
{
    const ITEM *item = nullptr;
    if (lua_type(L, 1) == LUA_TNUMBER) {
        const int idx = luaL_checkinteger(L, 1);
        item = Item_Get(idx - 1);
    } else {
        const char *const name = luaL_checkstring(L, 1);
        item = Item_GetByName(name);
    }
    if (item == nullptr) {
        lua_pushnil(L);
    } else {
        const ITEM **ud = lua_newuserdata(L, sizeof(ITEM *));
        *ud = item;
        luaL_getmetatable(L, "TRX.Items.ITEM");
        lua_setmetatable(L, -2);
    }
    return 1;
}

void LUA_CreateItems(lua_State *const L)
{
    // Register getter and setter functions in the Lua registry
    lua_newtable(L);
    luaL_setfuncs(L, m_ItemGetters, 0);
    lua_setfield(L, LUA_REGISTRYINDEX, "TRX.ItemGetters");

    lua_newtable(L);
    luaL_setfuncs(L, m_ItemSetters, 0);
    lua_setfield(L, LUA_REGISTRYINDEX, "TRX.ItemSetters");

    luaL_newmetatable(L, "TRX.Items.ITEM");
    lua_pushcfunction(L, M_L_ItemIndex);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, M_L_ItemNewIndex);
    lua_setfield(L, -2, "__newindex");
    lua_pop(L, 1);

    lua_getglobal(L, "TRX");
    lua_newtable(L);
    lua_pushcfunction(L, M_L_ItemsGet);
    lua_setfield(L, -2, "Get");
    lua_pushcfunction(L, M_L_ItemsCount);
    lua_setfield(L, -2, "Count");
    lua_setfield(L, -2, "Items");
    lua_pop(L, 1);
}
