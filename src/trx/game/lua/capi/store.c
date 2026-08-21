#include <trx/core/json/base.h>
#include <trx/core/log.h>
#include <trx/core/strings.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/store.h>
#include <trx/game/lua/utils.h>

#include <lauxlib.h>

typedef struct {
    int seen_idx;
    int32_t next_id;
} M_DUMP_CTX;

static lua_State *m_L = nullptr;
static int m_LevelRef = LUA_NOREF;
static int m_GameRef = LUA_NOREF;

static JSON_VALUE *M_DumpValue(
    lua_State *L, int idx, M_DUMP_CTX *ctx, const char *path);

// Empties a table without replacing it, so a script holding a reference to it
// keeps the one the store hands out.
static void M_ClearTable(lua_State *const L, const int ref)
{
    if (L == nullptr || ref == LUA_NOREF) {
        return;
    }
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        lua_pop(L, 1);
        lua_pushvalue(L, -1);
        lua_pushnil(L);
        lua_rawset(L, -4);
    }
    lua_pop(L, 1);
}

static const char *M_KeyPath(
    lua_State *const L, const char *const path, const int key_idx)
{
    if (lua_type(L, key_idx) == LUA_TSTRING) {
        return String_FormatStatic("%s.%s", path, lua_tostring(L, key_idx));
    }
    return String_FormatStatic(
        "%s[%g]", path, (double)lua_tonumber(L, key_idx));
}

static JSON_VALUE *M_DumpPairs(
    lua_State *const L, const int idx, M_DUMP_CTX *const ctx,
    const char *const path, const int32_t id)
{
    JSON_ARRAY *const pairs = JSON_ArrayNew();
    lua_pushnil(L);
    while (lua_next(L, idx) != 0) {
        const int key_idx = lua_gettop(L) - 1;
        const int value_idx = lua_gettop(L);
        const int key_type = lua_type(L, key_idx);
        if (key_type != LUA_TSTRING && key_type != LUA_TNUMBER) {
            LOG_WARNING("%s has a key that is not a string or a number", path);
            lua_pop(L, 1);
            continue;
        }

        const char *const key_path = M_KeyPath(L, path, key_idx);
        JSON_VALUE *const value = M_DumpValue(L, value_idx, ctx, key_path);
        if (value != nullptr) {
            JSON_ARRAY *const pair = JSON_ArrayNew();
            JSON_ArrayAppend(pair, M_DumpValue(L, key_idx, ctx, key_path));
            JSON_ArrayAppend(pair, value);
            JSON_ArrayAppendArray(pairs, pair);
        }
        lua_pop(L, 1);
    }

    JSON_OBJECT *const obj = JSON_ObjectNew();
    JSON_ObjectAppendInt(obj, "id", id);
    JSON_ObjectAppendArray(obj, "pairs", pairs);
    return JSON_ValueFromObject(obj);
}

// Writes a table the first time it is reached and gives it an id. Every later
// sighting of that same table writes the id instead. One table therefore comes
// back as one table, and a table that holds itself comes back whole.
static JSON_VALUE *M_DumpTable(
    lua_State *const L, const int idx, M_DUMP_CTX *const ctx,
    const char *const path)
{
    const void *const ptr = lua_topointer(L, idx);
    lua_rawgetp(L, ctx->seen_idx, ptr);
    if (!lua_isnil(L, -1)) {
        const int32_t seen_id = (int32_t)lua_tointeger(L, -1);
        lua_pop(L, 1);
        JSON_OBJECT *const ref = JSON_ObjectNew();
        JSON_ObjectAppendInt(ref, "ref", seen_id);
        return JSON_ValueFromObject(ref);
    }
    lua_pop(L, 1);

    const int32_t id = ctx->next_id++;
    lua_pushinteger(L, id);
    lua_rawsetp(L, ctx->seen_idx, ptr);
    return M_DumpPairs(L, idx, ctx, path, id);
}

static JSON_VALUE *M_DumpValue(
    lua_State *const L, const int idx, M_DUMP_CTX *const ctx,
    const char *const path)
{
    switch (lua_type(L, idx)) {
    case LUA_TBOOLEAN:
        return JSON_ValueFromBool(lua_toboolean(L, idx));

    case LUA_TNUMBER:
        if (lua_isinteger(L, idx)) {
            return JSON_ValueFromInt64(lua_tointeger(L, idx));
        }
        return JSON_ValueFromDouble(lua_tonumber(L, idx));

    case LUA_TSTRING:
        return JSON_ValueFromString(lua_tostring(L, idx));

    case LUA_TTABLE:
        return M_DumpTable(L, idx, ctx, path);

    default:
        LOG_WARNING(
            "%s holds a %s, which cannot be saved", path,
            lua_typename(L, lua_type(L, idx)));
        return nullptr;
    }
}

static JSON_VALUE *M_DumpStore(const int ref, const char *const name)
{
    lua_State *const L = m_L;
    if (L == nullptr || ref == LUA_NOREF) {
        return JSON_ValueFromObject(JSON_ObjectNew());
    }

    lua_newtable(L);
    M_DUMP_CTX ctx = {
        .seen_idx = lua_gettop(L),
        .next_id = 1,
    };
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
    JSON_VALUE *value = M_DumpTable(L, lua_gettop(L), &ctx, name);
    lua_pop(L, 2);
    if (value == nullptr) {
        value = JSON_ValueFromObject(JSON_ObjectNew());
    }
    return value;
}

static void M_PushJSON(lua_State *L, const JSON_VALUE *value, int ids_idx);

static void M_FillTable(
    lua_State *const L, const int table_idx, const JSON_ARRAY *const pairs,
    const int ids_idx)
{
    if (pairs == nullptr) {
        return;
    }
    for (size_t i = 0; i < pairs->length; i++) {
        const JSON_ARRAY *const pair = JSON_ArrayGetArray(pairs, i);
        if (pair == nullptr || pair->length != 2) {
            continue;
        }
        M_PushJSON(L, JSON_ArrayGetValue(pair, 0), ids_idx);
        M_PushJSON(L, JSON_ArrayGetValue(pair, 1), ids_idx);
        if (lua_isnil(L, -2)) {
            lua_pop(L, 2);
            continue;
        }
        lua_rawset(L, table_idx);
    }
}

static void M_PushJSONTable(
    lua_State *const L, const JSON_OBJECT *const obj, const int ids_idx)
{
    if (JSON_ObjectGetValue(obj, "ref") != nullptr) {
        lua_rawgeti(L, ids_idx, JSON_ObjectGetInt(obj, "ref", 0));
        return;
    }

    lua_newtable(L);
    const int id = JSON_ObjectGetInt(obj, "id", 0);
    if (id > 0) {
        lua_pushvalue(L, -1);
        lua_rawseti(L, ids_idx, id);
    }
    M_FillTable(L, lua_gettop(L), JSON_ObjectGetArray(obj, "pairs"), ids_idx);
}

static void M_PushJSON(
    lua_State *const L, const JSON_VALUE *const value, const int ids_idx)
{
    if (value == nullptr) {
        lua_pushnil(L);
        return;
    }

    switch ((JSON_TYPE)value->type) {
    case JSON_TYPE_TRUE:
        lua_pushboolean(L, true);
        break;

    case JSON_TYPE_FALSE:
        lua_pushboolean(L, false);
        break;

    case JSON_TYPE_NUMBER: {
        const double number = JSON_ValueGetDouble(value, 0.0);
        if (number == (double)(int64_t)number) {
            lua_pushinteger(L, (lua_Integer)number);
        } else {
            lua_pushnumber(L, number);
        }
        break;
    }

    case JSON_TYPE_STRING:
        lua_pushstring(L, JSON_ValueGetString(value, ""));
        break;

    case JSON_TYPE_OBJECT:
        M_PushJSONTable(L, JSON_ValueAsObject(value), ids_idx);
        break;

    default:
        lua_pushnil(L);
        break;
    }
}

static void M_LoadStore(const int ref, const JSON_OBJECT *const obj)
{
    lua_State *const L = m_L;
    if (L == nullptr || ref == LUA_NOREF) {
        return;
    }

    M_ClearTable(L, ref);
    if (obj == nullptr) {
        return;
    }

    lua_newtable(L);
    const int ids_idx = lua_gettop(L);
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
    const int store_idx = lua_gettop(L);
    const int id = JSON_ObjectGetInt(obj, "id", 0);
    if (id > 0) {
        lua_pushvalue(L, store_idx);
        lua_rawseti(L, ids_idx, id);
    }
    M_FillTable(L, store_idx, JSON_ObjectGetArray(obj, "pairs"), ids_idx);
    lua_pop(L, 2);
}

static void M_Create(lua_State *const L)
{
    m_L = L;

    static const luaL_Reg empty[] = { { nullptr, nullptr } };
    LUA_RegisterModule(L, "store", empty);
    LUA_GetModule(L, "store");

    lua_newtable(L);
    lua_pushvalue(L, -1);
    m_LevelRef = luaL_ref(L, LUA_REGISTRYINDEX);
    lua_setfield(L, -2, "level");

    lua_newtable(L);
    lua_pushvalue(L, -1);
    m_GameRef = luaL_ref(L, LUA_REGISTRYINDEX);
    lua_setfield(L, -2, "game");

    lua_pop(L, 1);
}

static void M_Shutdown(void)
{
    m_L = nullptr;
    m_LevelRef = LUA_NOREF;
    m_GameRef = LUA_NOREF;
}

void LUA_Store_ClearLevel(void)
{
    M_ClearTable(m_L, m_LevelRef);
}

void LUA_Store_ClearGame(void)
{
    M_ClearTable(m_L, m_GameRef);
}

void LUA_Store_Dump(JSON_WRITE_IO *const io)
{
    JSONW_PUSH_OBJECT(io);
    JSON_OBJECT *const obj = JSON_WriteIO_GetCurrentObject(io);
    JSON_ObjectAppend(obj, "level", M_DumpStore(m_LevelRef, "level"));
    JSON_ObjectAppend(obj, "game", M_DumpStore(m_GameRef, "game"));
    JSONW_POP_AND_SET(io, "store");
}

RESULT LUA_Store_Load(JSON_READ_IO *const io)
{
    // A save without a store section leaves both stores empty.
    if (!JSON_ReadIO_HasKey(io, "store")) {
        LUA_Store_ClearLevel();
        LUA_Store_ClearGame();
        return OK;
    }

    MUST(JSON_PUSH(io, "store"));
    const JSON_OBJECT *const obj = JSON_ReadIO_GetCurrentObject(io);
    M_LoadStore(m_LevelRef, JSON_ObjectGetObject(obj, "level"));
    M_LoadStore(m_GameRef, JSON_ObjectGetObject(obj, "game"));
    MUST(JSON_POP(io));
    return OK;
}

REGISTER_LUA_CAPI(.create = M_Create, .shutdown = M_Shutdown)
