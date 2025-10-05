#include "game/items.h"
#include "game/lua/common.h"
#include "game/rooms.h"
#include "utils.h"

#include <lauxlib.h>
#include <string.h>

// Lua metamethod: __index for TRX.Items.ITEM read fields
static int M_L_ItemIndex(lua_State *const L)
{
    const ITEM *const item = *(ITEM **)luaL_checkudata(L, 1, "TRX.Items.ITEM");
    const char *const key = luaL_checkstring(L, 2);
    if (strcmp(key, "pos") == 0) {
        lua_newtable(L);
        lua_pushinteger(L, item->pos.x);
        lua_setfield(L, -2, "x");
        lua_pushinteger(L, item->pos.y);
        lua_setfield(L, -2, "y");
        lua_pushinteger(L, item->pos.z);
        lua_setfield(L, -2, "z");
        return 1;
    } else if (strcmp(key, "rot") == 0) {
        lua_newtable(L);
        lua_pushinteger(L, item->rot.x);
        lua_setfield(L, -2, "x");
        lua_pushinteger(L, item->rot.y);
        lua_setfield(L, -2, "y");
        lua_pushinteger(L, item->rot.z);
        lua_setfield(L, -2, "z");
        return 1;
    } else if (strcmp(key, "room") == 0) {
        lua_pushinteger(L, item->room_num);
        return 1;
    } else if (strcmp(key, "status") == 0) {
        lua_pushinteger(L, (int)item->status);
        return 1;
    } else if (strcmp(key, "object_id") == 0) {
        lua_pushinteger(L, item->object_id);
        return 1;
    } else if (strcmp(key, "hit_points") == 0) {
        lua_pushinteger(L, item->hit_points);
        return 1;
    } else if (strcmp(key, "max_hit_points") == 0) {
        lua_pushinteger(L, item->max_hit_points);
        return 1;
    } else if (strcmp(key, "name") == 0) {
        if (item->name != nullptr) {
            lua_pushstring(L, item->name);
        } else {
            lua_pushnil(L);
        }
        return 1;
    }
    lua_pushnil(L);
    return 1;
}

// Lua metamethod: __newindex for writable fields
static int M_L_ItemNewIndex(lua_State *const L)
{
    ITEM *item = *(ITEM **)luaL_checkudata(L, 1, "TRX.Items.ITEM");
    const char *key = luaL_checkstring(L, 2);
    if (strcmp(key, "pos") == 0) {
        luaL_checktype(L, 3, LUA_TTABLE);
        lua_getfield(L, 3, "x");
        item->pos.x = luaL_checkinteger(L, -1);
        lua_pop(L, 1);
        lua_getfield(L, 3, "y");
        item->pos.y = luaL_checkinteger(L, -1);
        lua_pop(L, 1);
        lua_getfield(L, 3, "z");
        item->pos.z = luaL_checkinteger(L, -1);
        lua_pop(L, 1);
        item->room_num =
            Room_GetIndexFromPos(item->pos.x, item->pos.y, item->pos.z);
        return 0;
    } else if (strcmp(key, "hit_points") == 0) {
        item->hit_points = luaL_checkinteger(L, 3);
        item->max_hit_points = MAX(item->hit_points, item->max_hit_points);
        return 0;
    } else if (strcmp(key, "max_hit_points") == 0) {
        item->max_hit_points = luaL_checkinteger(L, 3);
        return 0;
    } else if (strcmp(key, "rot") == 0) {
        luaL_checktype(L, 3, LUA_TTABLE);
        lua_getfield(L, 3, "x");
        item->rot.x = luaL_checkinteger(L, -1);
        lua_pop(L, 1);
        lua_getfield(L, 3, "y");
        item->rot.y = luaL_checkinteger(L, -1);
        lua_pop(L, 1);
        lua_getfield(L, 3, "z");
        item->rot.z = luaL_checkinteger(L, -1);
        lua_pop(L, 1);
        return 0;
    } else if (strcmp(key, "name") == 0) {
        const char *const new_name = luaL_checkstring(L, 3);
        if (!Item_SetName(Item_GetIndex(item), new_name)) {
            return luaL_error(L, "item name '%s' already in use", new_name);
        }
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

// item = TRX.Items.Get(idx)
static int M_L_ItemsGet(lua_State *const L)
{
    int idx = luaL_checkinteger(L, 1);
    const ITEM *const item = Item_Get(idx - 1);
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

// item = TRX.Items.GetByName(name)
static int M_L_ItemsGetByName(lua_State *const L)
{
    const char *const name = luaL_checkstring(L, 1);
    ITEM *const item = Item_GetByName(name);
    if (item == nullptr) {
        lua_pushnil(L);
    } else {
        ITEM **ud = lua_newuserdata(L, sizeof(ITEM *));
        *ud = item;
        luaL_getmetatable(L, "TRX.Items.ITEM");
        lua_setmetatable(L, -2);
    }
    return 1;
}

void LUA_CreateItems(lua_State *const L)
{
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
    lua_pushcfunction(L, M_L_ItemsGetByName);
    lua_setfield(L, -2, "GetByName");
    lua_pushcfunction(L, M_L_ItemsCount);
    lua_setfield(L, -2, "Count");
    lua_setfield(L, -2, "Items");
    lua_pop(L, 1);
}
