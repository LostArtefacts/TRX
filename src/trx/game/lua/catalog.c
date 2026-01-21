#include <lauxlib.h>
#include <stdint.h>

static void M_PushObjects(lua_State *const L)
{
    lua_newtable(L);

    int32_t id = 0;
#define X_CATALOG_ID(enum_value)                                               \
    lua_pushinteger(L, id++);                                                  \
    lua_setfield(L, -2, #enum_value);
#include "trx/game/catalog_objects.def"
#undef X_CATALOG_ID

    lua_setfield(L, -2, "objects");
}

static void M_PushFlipEffects(lua_State *const L)
{
    lua_newtable(L);

    int32_t id = 0;
#define X_CATALOG_ID(enum_value)                                               \
    lua_pushinteger(L, id++);                                                  \
    lua_setfield(L, -2, #enum_value);
#include "trx/game/catalog_item_actions.def"
#undef X_CATALOG_ID

    lua_setfield(L, -2, "flip_effects");
}

void LUA_CreateCatalog(lua_State *const L)
{
    lua_getglobal(L, "trxc");
    lua_newtable(L);

    M_PushObjects(L);
    M_PushFlipEffects(L);

    lua_setfield(L, -2, "catalog");
    lua_pop(L, 1);
}
