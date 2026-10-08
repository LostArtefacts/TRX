#pragma once

#include <trx/game/lua/utils/args.h>
#include <trx/game/savegame.h>

#include <lauxlib.h>

// Resolve a script slot number and pool to a save slot. Number quick saves by
// the order of slots that contain saves.
static inline SAVEGAME_SLOT_REF LUA_ResolveSaveSlot(
    int32_t slot_num, SAVEGAME_SLOT_POOL pool)
{
    if (pool == SAVEGAME_SLOT_POOL_QUICK) {
        return SG_Manager_QuickFromVisualIndex(slot_num - 1);
    }
    return SG_Manager_NormalSlot(slot_num - 1);
}

static inline SAVEGAME_SLOT_POOL LUA_CheckSavePool(lua_State *L, int arg)
{
    return (SAVEGAME_SLOT_POOL)LUA_CheckRange(
        L, arg, SAVEGAME_SLOT_POOL_NUMBER_OF, "unknown save pool");
}
