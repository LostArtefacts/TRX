#include <trx/game/objects.h>

#include <lauxlib.h>

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

void LUA_CreateCatalog(lua_State *const L)
{
    lua_getglobal(L, "trxc");
    lua_newtable(L);

    M_PushObjects(L);

    lua_setfield(L, -2, "catalog");
    lua_pop(L, 1);
}
