// Lua binding for game module
#include "game/game_flow/common.h"
#include "game/lua/common.h"
#include "version.h"

#include <lauxlib.h>

// trxc.game.get_version() → int
static int M_L_GameVersion(lua_State *const L)
{
    lua_pushinteger(L, g_TRVersion);
    return 1;
}

// trxc.game.get_trx_version() → string
static int M_L_TRXVersion(lua_State *const L)
{
    lua_pushstring(L, g_TRXVersion);
    return 1;
}

// trxc.game.count_levels() → int
static int M_L_GameCountLevels(lua_State *const L)
{
    const GF_LEVEL_TABLE_TYPE table_type = luaL_checkinteger(L, 1);
    lua_pushinteger(L, GF_GetLevelCount(table_type));
    return 1;
}

// Level property getters
static int M_L_GameLevelGetNum(lua_State *const L)
{
    const GF_LEVEL_TABLE_TYPE table_type = luaL_checkinteger(L, 1);
    const int32_t idx = luaL_checkinteger(L, 2);
    const GF_LEVEL *const lvl = GF_GetLevel(table_type, idx - 1);
    lua_pushinteger(
        L, lvl != nullptr ? GF_GetLevelOrdinalNumber(table_type, lvl) : 0);
    return 1;
}

static int M_L_GameLevelGetName(lua_State *const L)
{
    const GF_LEVEL_TABLE_TYPE table_type = luaL_checkinteger(L, 1);
    const int32_t idx = luaL_checkinteger(L, 2);
    const GF_LEVEL *const lvl = GF_GetLevel(table_type, idx - 1);
    if (lvl != nullptr && lvl->title != nullptr) {
        lua_pushstring(L, lvl->title);
    } else {
        lua_pushnil(L);
    }
    return 1;
}

static int M_L_GameLevelGetPath(lua_State *const L)
{
    const GF_LEVEL_TABLE_TYPE table_type = luaL_checkinteger(L, 1);
    const int32_t idx = luaL_checkinteger(L, 2);
    const GF_LEVEL *const lvl = GF_GetLevel(table_type, idx - 1);
    if (lvl != nullptr && lvl->path != nullptr) {
        lua_pushstring(L, lvl->path);
    } else {
        lua_pushnil(L);
    }
    return 1;
}

static int M_L_GameLevelGetType(lua_State *const L)
{
    const GF_LEVEL_TABLE_TYPE table_type = luaL_checkinteger(L, 1);
    const int32_t idx = luaL_checkinteger(L, 2);
    const GF_LEVEL *const lvl = GF_GetLevel(table_type, idx - 1);
    lua_pushinteger(L, lvl != nullptr ? lvl->type : 0);
    return 1;
}

static int M_L_GameLevelGetCurrentLevelTable(lua_State *const L)
{
    const GF_LEVEL *const lvl = GF_GetCurrentLevel();
    lua_pushinteger(
        L, lvl != nullptr ? GF_GetLevelTableType(lvl->type) : GFLT_UNKNOWN);
    return 1;
}

static int M_L_GameLevelGetCurrentLevelIndex(lua_State *const L)
{
    const GF_LEVEL *const lvl = GF_GetCurrentLevel();
    lua_pushinteger(L, lvl != nullptr ? lvl->num : -1);
    return 1;
}

void LUA_CreateGame(lua_State *const L)
{
    lua_getglobal(L, "trxc");
    lua_newtable(L);

    lua_pushcfunction(L, M_L_GameVersion);
    lua_setfield(L, -2, "get_version");
    lua_pushcfunction(L, M_L_TRXVersion);
    lua_setfield(L, -2, "get_trx_version");
    lua_pushcfunction(L, M_L_GameCountLevels);
    lua_setfield(L, -2, "count_levels");
    lua_pushcfunction(L, M_L_GameLevelGetNum);
    lua_setfield(L, -2, "get_level_num");
    lua_pushcfunction(L, M_L_GameLevelGetName);
    lua_setfield(L, -2, "get_level_name");
    lua_pushcfunction(L, M_L_GameLevelGetPath);
    lua_setfield(L, -2, "get_level_path");
    lua_pushcfunction(L, M_L_GameLevelGetType);
    lua_setfield(L, -2, "get_level_type");
    lua_pushcfunction(L, M_L_GameLevelGetCurrentLevelTable);
    lua_setfield(L, -2, "get_current_level_table");
    lua_pushcfunction(L, M_L_GameLevelGetCurrentLevelIndex);
    lua_setfield(L, -2, "get_current_level_idx");

    lua_newtable(L);
    lua_pushinteger(L, GFLT_MAIN);
    lua_setfield(L, -2, "MAIN");
    lua_pushinteger(L, GFLT_CUTSCENES);
    lua_setfield(L, -2, "CUTSCENES");
    lua_pushinteger(L, GFLT_DEMOS);
    lua_setfield(L, -2, "DEMOS");
    lua_setfield(L, -2, "LevelTable");

    lua_newtable(L);
    lua_pushinteger(L, GFL_NORMAL);
    lua_setfield(L, -2, "NORMAL");
    lua_pushinteger(L, GFL_CUTSCENE);
    lua_setfield(L, -2, "CUTSCENE");
    lua_pushinteger(L, GFL_DEMO);
    lua_setfield(L, -2, "DEMO");
    lua_pushinteger(L, GFL_GYM);
    lua_setfield(L, -2, "GYM");
    lua_pushinteger(L, GFL_BONUS);
    lua_setfield(L, -2, "BONUS");
    lua_setfield(L, -2, "LevelType");

    lua_setfield(L, -2, "game");
    lua_pop(L, 1);
}
