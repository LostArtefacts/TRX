#include <trx/core/result.h>
#include <trx/game/catalog/manager.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>

#include <lauxlib.h>
#include <stdint.h>
#include <string.h>

// Report each catalogue's run-time identities directly because build-time
// ENUM_MAP entries exclude identities created by mods.
//
// Convert between a TRX ID and the game-specific slot used by its files and
// Tomb Editor; TRX IDs span all four games, while slots vary by game.

// Neither Catalog_ToSlot nor Catalog_FromSlot checks the context before
// indexing its table with it.
static CATALOG_CONTEXT M_CheckContext(lua_State *const L, const int arg)
{
    return (CATALOG_CONTEXT)LUA_CheckRange(
        L, arg, CATALOG_CONTEXT_MAX, "unknown catalog context");
}

// trxc.catalog.mint(context, name) -> int or nil
static int M_L_CatalogMint(lua_State *const L)
{
    const CATALOG_CONTEXT context = M_CheckContext(L, 1);
    const char *const key = luaL_checkstring(L, 2);
    CATALOG_ID id;
    if (!IS_OK(Catalog_Mint(context, key, &id))) {
        lua_pushnil(L);
    } else {
        lua_pushinteger(L, id);
    }
    return 1;
}

// trxc.catalog.key(context, id) -> string or nil
static int M_L_CatalogKey(lua_State *const L)
{
    const CATALOG_CONTEXT context = M_CheckContext(L, 1);
    CATALOG_ID id;
    if (!LUA_CheckBoundedInt(L, 2, INT32_MIN, INT32_MAX, &id)) {
        lua_pushnil(L);
        return 1;
    }
    const char *const key = Catalog_GetKey(context, id);
    if (key == nullptr) {
        lua_pushnil(L);
    } else {
        lua_pushstring(L, key);
    }
    return 1;
}

// trxc.catalog.to_slot(context, id) -> int or nil
static int M_L_CatalogToSlot(lua_State *const L)
{
    const CATALOG_CONTEXT context = M_CheckContext(L, 1);
    CATALOG_ID id;
    // Whether the narrowed id is in the catalog at all is Catalog_*'s to say.
    if (!LUA_CheckBoundedInt(L, 2, INT32_MIN, INT32_MAX, &id)) {
        lua_pushnil(L);
        return 1;
    }
    const int32_t slot = Catalog_ToSlot(context, id, -1);
    if (slot < 0) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushinteger(L, slot);
    return 1;
}

// trxc.catalog.from_slot(context, slot) -> int or nil
static int M_L_CatalogFromSlot(lua_State *const L)
{
    const CATALOG_CONTEXT context = M_CheckContext(L, 1);
    int32_t slot;
    if (!LUA_CheckBoundedInt(L, 2, INT32_MIN, INT32_MAX, &slot)) {
        lua_pushnil(L);
        return 1;
    }
    const CATALOG_ID id = Catalog_FromSlot(context, slot, -1);
    if (id < 0) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushinteger(L, id);
    return 1;
}

// trxc.catalog.values(context) -> { { name = "shotgun_item", value = 1 }, ... }
static int M_L_CatalogValues(lua_State *const L)
{
    const CATALOG_CONTEXT context = M_CheckContext(L, 1);
    const int32_t count = Catalog_GetCount(context);
    lua_createtable(L, count, 0);
    int32_t n = 0;
    for (CATALOG_ID id = 0; id < count; id++) {
        const char *const key = Catalog_GetKey(context, id);
        if (key == nullptr) {
            continue;
        }
        lua_createtable(L, 0, 2);
        lua_pushstring(L, key);
        lua_setfield(L, -2, "name");
        lua_pushinteger(L, id);
        lua_setfield(L, -2, "value");
        lua_rawseti(L, -2, ++n);
    }
    return 1;
}

// trxc.catalog.from_key(context, name) -> int or nil
static int M_L_CatalogFromKey(lua_State *const L)
{
    const CATALOG_CONTEXT context = M_CheckContext(L, 1);
    const char *const key = luaL_checkstring(L, 2);
    const CATALOG_ID id = Catalog_FromKey(context, key, -1);
    // Return a match only for the canonical key, because aliases are C
    // spellings used by catalogue CSVs rather than script names.
    const char *const canonical =
        id < 0 ? nullptr : Catalog_GetKey(context, id);
    if (canonical == nullptr || strcmp(canonical, key) != 0) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushinteger(L, id);
    return 1;
}

static const luaL_Reg m_Module[] = {
    { "key", M_L_CatalogKey },
    { "mint", M_L_CatalogMint },
    { "values", M_L_CatalogValues },
    { "from_key", M_L_CatalogFromKey },
    { "to_slot", M_L_CatalogToSlot },
    { "from_slot", M_L_CatalogFromSlot },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "catalog", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
