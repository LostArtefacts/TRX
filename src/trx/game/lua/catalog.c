#include <trx/memory.h>

#include <ctype.h>
#include <lauxlib.h>
#include <stdint.h>
#include <string.h>

static void M_PushCatalogKey(
    lua_State *const L, const char *const name, const char *const prefix,
    const int32_t value)
{
    const char *key = name;
    if (prefix != nullptr) {
        const size_t prefix_len = strlen(prefix);
        if (strncmp(name, prefix, prefix_len) == 0) {
            key = name + prefix_len;
        }
    }

    const size_t key_len = strlen(key);
    char *const lower_key = Memory_Alloc(key_len + 1);
    for (size_t i = 0; i < key_len; i++) {
        lower_key[i] = (char)tolower((unsigned char)key[i]);
    }
    lower_key[key_len] = '\0';

    lua_pushinteger(L, value);
    lua_setfield(L, -2, lower_key);
    Memory_Free(lower_key);
}

static void M_PushObjects(lua_State *const L)
{
    lua_newtable(L);

    int32_t id = 0;
#define X_CATALOG_ID(enum_value) M_PushCatalogKey(L, #enum_value, "O_", id++);
#include "trx/game/catalog_objects.def"
#undef X_CATALOG_ID

    lua_setfield(L, -2, "objects");
}

static void M_PushFlipEffects(lua_State *const L)
{
    lua_newtable(L);

    int32_t id = 0;
#define X_CATALOG_ID(enum_value)                                               \
    M_PushCatalogKey(L, #enum_value, "ITEM_ACTION_", id++);
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
