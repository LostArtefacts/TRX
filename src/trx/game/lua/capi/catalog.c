#include <trx/game/catalog/manager.h>
#include <trx/game/lua/registry.h>

#include <lauxlib.h>
#include <stdint.h>

// The catalogs themselves are enums, reflected out of ENUM_MAP like any other -
// see trx/game/enum.c. What is left here is the one thing an enum cannot do:
// turn a TRX id into the id this particular game's files use, and back.
//
// Builders call that a slot. It is what Tomb Editor shows, and it is not the
// same number as the TRX id: TRX names an object once, across all four games,
// and each game's files number it differently.

// trxc.catalog.to_slot(context, id) -> int or nil
static int M_L_CatalogToSlot(lua_State *const L)
{
    const CATALOG_CONTEXT context = luaL_checkinteger(L, 1);
    const CATALOG_ID id = luaL_checkinteger(L, 2);
    int32_t game_id;
    if (!Catalog_EnumToGameID(context, id, &game_id)) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushinteger(L, game_id);
    return 1;
}

// trxc.catalog.from_slot(context, slot) -> int or nil
static int M_L_CatalogFromSlot(lua_State *const L)
{
    const CATALOG_CONTEXT context = luaL_checkinteger(L, 1);
    const int32_t game_id = luaL_checkinteger(L, 2);
    CATALOG_ID id;
    if (!Catalog_GameIDToEnum(context, game_id, &id)) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushinteger(L, id);
    return 1;
}

static void M_Create(lua_State *const L)
{
    lua_getglobal(L, "trxc");
    lua_newtable(L);
    lua_pushcfunction(L, M_L_CatalogToSlot);
    lua_setfield(L, -2, "to_slot");
    lua_pushcfunction(L, M_L_CatalogFromSlot);
    lua_setfield(L, -2, "from_slot");
    lua_setfield(L, -2, "catalog");
    lua_pop(L, 1);
}

REGISTER_LUA_CAPI(.create = M_Create)
