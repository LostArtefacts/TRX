#include <trx/game/catalog/manager.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>

#include <lauxlib.h>
#include <stdint.h>

// The catalogs themselves are enums, reflected out of ENUM_MAP like any other -
// see trx/game/enum.c. What is left here is the one thing an enum cannot do:
// turn a TRX id into the id this particular game's files use, and back.
//
// Builders call that a slot. It is what Tomb Editor shows, and it is not the
// same number as the TRX id: TRX names an object once, across all four games,
// and each game's files number it differently.

// Neither Catalog_ToSlot nor Catalog_FromSlot checks the context before
// indexing its table with it.
static CATALOG_CONTEXT M_CheckContext(lua_State *const L, const int arg)
{
    return (CATALOG_CONTEXT)LUA_CheckRange(
        L, arg, CATALOG_CONTEXT_MAX, "unknown catalog context");
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

static const luaL_Reg m_Module[] = {
    { "to_slot", M_L_CatalogToSlot },
    { "from_slot", M_L_CatalogFromSlot },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "catalog", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
