#include "game/lua/structs.h"

#include "game/items.h"

#include <lauxlib.h>
#include <string.h>

static int M_L_ItemIndex(lua_State *l);

static int M_L_ItemIndex(lua_State *const l)
{
    ITEM *item = *(ITEM **)luaL_checkudata(l, 1, "TRX.ITEM");
    const char *key = luaL_checkstring(l, 2);
    if (strcmp(key, "pos") == 0) {
        lua_newtable(l);
        lua_pushinteger(l, item->pos.x);
        lua_setfield(l, -2, "x");
        lua_pushinteger(l, item->pos.y);
        lua_setfield(l, -2, "y");
        lua_pushinteger(l, item->pos.z);
        lua_setfield(l, -2, "z");
        return 1;
    } else if (strcmp(key, "rot") == 0) {
        lua_newtable(l);
        lua_pushinteger(l, item->rot.x);
        lua_setfield(l, -2, "x");
        lua_pushinteger(l, item->rot.y);
        lua_setfield(l, -2, "y");
        lua_pushinteger(l, item->rot.z);
        lua_setfield(l, -2, "z");
        return 1;
    } else if (strcmp(key, "room") == 0) {
        lua_pushinteger(l, item->room_num);
        return 1;
    } else if (strcmp(key, "status") == 0) {
        lua_pushinteger(l, (int)item->status);
        return 1;
    } else if (strcmp(key, "hit_points") == 0) {
        lua_pushinteger(l, item->hit_points);
        return 1;
    }
    lua_pushnil(l);
    return 1;
}

void LUA_CreateStructs(lua_State *const l)
{
    luaL_newmetatable(l, "TRX.ITEM");
    lua_pushcfunction(l, M_L_ItemIndex);
    lua_setfield(l, -2, "__index");
    lua_pop(l, 1);
}
