#include <trx/core/result.h>
#include <trx/game/game_flow.h>
#include <trx/game/game_flow/types.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/savegame.h>
#include <trx/game/lua/utils.h>
#include <trx/game/savegame.h>

#include <lauxlib.h>

// trxc.savegame.slot_count(pool) -> int
static int M_L_SavegameSlotCount(lua_State *const L)
{
    const SAVEGAME_SLOT_POOL pool = LUA_CheckSavePool(L, 1);
    if (pool == SAVEGAME_SLOT_POOL_QUICK) {
        lua_pushinteger(L, SG_Manager_GetQuickVisualCount());
    } else {
        lua_pushinteger(L, SG_Manager_GetSlotCount(pool));
    }
    return 1;
}

// trxc.savegame.is_free(index, pool) -> bool
static int M_L_SavegameIsFree(lua_State *const L)
{
    const int32_t index = luaL_checkinteger(L, 1);
    const SAVEGAME_SLOT_POOL pool = LUA_CheckSavePool(L, 2);
    lua_pushboolean(L, SG_Manager_IsSlotFree(LUA_ResolveSaveSlot(index, pool)));
    return 1;
}

// trxc.savegame.info(index, pool) -> table or nil
// Return the save slot details. Return nil for a free slot.
static int M_L_SavegameInfo(lua_State *const L)
{
    const int32_t index = luaL_checkinteger(L, 1);
    const SAVEGAME_SLOT_POOL pool = LUA_CheckSavePool(L, 2);
    const SAVEGAME_SLOT_REF slot = LUA_ResolveSaveSlot(index, pool);
    if (!SG_Manager_IsValidSlotRef(slot) || SG_Manager_IsSlotFree(slot)) {
        lua_pushnil(L);
        return 1;
    }
    const SAVEGAME_INFO *const info = SG_Manager_GetSavegameInfo(slot);
    lua_newtable(L);
    lua_pushstring(L, info->level_title);
    lua_setfield(L, -2, "level_title");
    lua_pushinteger(L, info->counter);
    lua_setfield(L, -2, "counter");
    lua_pushinteger(L, info->level_num);
    lua_setfield(L, -2, "level_num");
    lua_pushboolean(L, info->is_quick);
    lua_setfield(L, -2, "is_quick");
    lua_pushboolean(L, info->features.restart);
    lua_setfield(L, -2, "can_restart");
    lua_pushboolean(L, info->features.select_level);
    lua_setfield(L, -2, "can_select_level");
    lua_pushboolean(L, GF_HasAvailableStory(slot));
    lua_setfield(L, -2, "has_story");
    return 1;
}

// trxc.savegame.delete(index, pool) -> bool
static int M_L_SavegameDelete(lua_State *const L)
{
    const int32_t index = luaL_checkinteger(L, 1);
    const SAVEGAME_SLOT_POOL pool = LUA_CheckSavePool(L, 2);
    const SAVEGAME_SLOT_REF slot = LUA_ResolveSaveSlot(index, pool);
    lua_pushboolean(
        L, SG_Manager_IsValidSlotRef(slot) && SG_Manager_Delete(slot));
    return 1;
}

// trxc.savegame.play_story(index, pool)
static int M_L_SavegamePlayStory(lua_State *const L)
{
    const int32_t index = luaL_checkinteger(L, 1);
    const SAVEGAME_SLOT_POOL pool = LUA_CheckSavePool(L, 2);
    const SAVEGAME_SLOT_REF slot = LUA_ResolveSaveSlot(index, pool);
    if (!SG_Manager_IsValidSlotRef(slot) || SG_Manager_IsSlotFree(slot)) {
        return luaL_error(L, "no saved game in this slot");
    }
    if (!GF_HasAvailableStory(slot)) {
        return luaL_error(L, "no story runs before this save");
    }
    GF_OverrideCommand(
        (GF_COMMAND) {
            .action = GF_STORY_SO_FAR,
            .param = SG_Manager_SlotToParam(slot),
        },
        true);
    return 0;
}

// trxc.savegame.total_count() -> int
static int M_L_SavegameTotalCount(lua_State *const L)
{
    lua_pushinteger(L, SG_Manager_GetTotalCount());
    return 1;
}

// trxc.savegame.restart_available(index, pool) -> bool
static int M_L_SavegameRestartAvailable(lua_State *const L)
{
    if (lua_isnoneornil(L, 1)) {
        lua_pushboolean(
            L, Savegame_RestartAvailable(SG_Manager_GetBoundSlot()));
        return 1;
    }
    const SAVEGAME_SLOT_REF slot =
        LUA_ResolveSaveSlot(luaL_checkinteger(L, 1), LUA_CheckSavePool(L, 2));
    lua_pushboolean(
        L, SG_Manager_IsValidSlotRef(slot) && Savegame_RestartAvailable(slot));
    return 1;
}

// trxc.savegame.reached_levels(index, pool) -> {int} or nil
static int M_L_SavegameReachedLevels(lua_State *const L)
{
    const int32_t index = luaL_checkinteger(L, 1);
    const SAVEGAME_SLOT_POOL pool = LUA_CheckSavePool(L, 2);
    const SAVEGAME_SLOT_REF slot = LUA_ResolveSaveSlot(index, pool);
    if (!SG_Manager_IsValidSlotRef(slot) || SG_Manager_IsSlotFree(slot)) {
        lua_pushnil(L);
        return 1;
    }
    VECTOR *const levels = Vector_Create(sizeof(const GF_LEVEL *));
    if (!SHOULD(Savegame_ReadReachedLevels(slot, levels))) {
        Vector_Free(levels);
        lua_pushnil(L);
        return 1;
    }
    lua_createtable(L, levels->count, 0);
    int32_t count = 0;
    for (int32_t i = 0; i < levels->count; i++) {
        const GF_LEVEL *const level = *(const GF_LEVEL **)Vector_Get(levels, i);
        const int32_t num = GF_GetLevelOrdinalNumber(GFLT_MAIN, level);
        if (num > 0) {
            lua_pushinteger(L, num);
            lua_rawseti(L, -2, ++count);
        }
    }
    Vector_Free(levels);
    return 1;
}

// trxc.savegame.manual_allowed() -> bool
static int M_L_SavegameManualAllowed(lua_State *const L)
{
    lua_pushboolean(L, Savegame_IsManualSaveAllowed());
    return 1;
}

// trxc.savegame.recent_slot() -> index, pool
// Select the running slot, then the most recently written slot, then the first
// numbered slot.
static int M_L_SavegameRecentSlot(lua_State *const L)
{
    SAVEGAME_SLOT_REF slot = SG_Manager_GetMostRecentlyUsedSlot();
    if (!SG_Manager_IsValidSlotRef(slot)) {
        slot = SG_Manager_GetMostRecentlyCreatedSlot();
    }
    if (!SG_Manager_IsValidSlotRef(slot)) {
        slot = SG_Manager_NormalSlot(0);
    }
    if (!SG_Manager_IsValidSlotRef(slot)) {
        lua_pushnil(L);
        return 1;
    }
    if (slot.pool == SAVEGAME_SLOT_POOL_QUICK) {
        lua_pushinteger(L, SG_Manager_QuickToVisualIndex(slot) + 1);
    } else {
        lua_pushinteger(L, slot.index + 1);
    }
    lua_pushinteger(L, slot.pool);
    return 2;
}

// trxc.savegame.load(index, pool)
static int M_L_SavegameLoad(lua_State *const L)
{
    const int32_t index = luaL_checkinteger(L, 1);
    const SAVEGAME_SLOT_POOL pool = LUA_CheckSavePool(L, 2);
    const SAVEGAME_SLOT_REF slot = LUA_ResolveSaveSlot(index, pool);
    if (!SG_Manager_IsValidSlotRef(slot) || SG_Manager_IsSlotFree(slot)) {
        return luaL_error(L, "no saved game in this slot");
    }
    GF_OverrideCommand(
        (GF_COMMAND) {
            .action = GF_START_SAVED_GAME,
            .param = SG_Manager_SlotToParam(slot),
        },
        true);
    return 0;
}

// trxc.savegame.save(index, pool) -> bool
static int M_L_SavegameSave(lua_State *const L)
{
    const SAVEGAME_SLOT_POOL pool = LUA_CheckSavePool(L, 2);

    SAVEGAME_SLOT_REF slot;
    if (pool == SAVEGAME_SLOT_POOL_QUICK && lua_isnoneornil(L, 1)) {
        slot = SG_Manager_GetNextQuickSlot();
        if (!SG_Manager_IsValidSlotRef(slot)) {
            lua_pushboolean(L, false);
            return 1;
        }
    } else {
        slot = LUA_ResolveSaveSlot(luaL_checkinteger(L, 1), pool);
    }

    lua_pushboolean(L, Savegame_Save(slot));
    return 1;
}

static const luaL_Reg m_Module[] = {
    { "slot_count", M_L_SavegameSlotCount },
    { "is_free", M_L_SavegameIsFree },
    { "info", M_L_SavegameInfo },
    { "total_count", M_L_SavegameTotalCount },
    { "restart_available", M_L_SavegameRestartAvailable },
    { "reached_levels", M_L_SavegameReachedLevels },
    { "manual_allowed", M_L_SavegameManualAllowed },
    { "recent_slot", M_L_SavegameRecentSlot },
    { "delete", M_L_SavegameDelete },
    { "play_story", M_L_SavegamePlayStory },
    { "load", M_L_SavegameLoad },
    { "save", M_L_SavegameSave },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "savegame", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
