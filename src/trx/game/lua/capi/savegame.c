#include <trx/game/game_flow.h>
#include <trx/game/game_flow/types.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/utils.h>
#include <trx/game/savegame.h>

#include <lauxlib.h>

// A save slot is a value, not a lasting object, so it is not handed to Lua as a
// handle; a script names a slot by its 1-based number and a pool, and the slot
// is resolved on each call.
static SAVEGAME_SLOT_REF M_ResolveSlot(
    const int32_t index, const SAVEGAME_SLOT_POOL pool)
{
    if (pool == SAVEGAME_SLOT_POOL_QUICK) {
        // The quick pool is addressed by its on-screen order, which counts only
        // the slots that hold a save.
        return Savegame_QuickFromVisualIndex(index - 1);
    }
    return Savegame_NormalSlot(index - 1);
}

static SAVEGAME_SLOT_POOL M_CheckPool(lua_State *const L, const int arg)
{
    return (SAVEGAME_SLOT_POOL)LUA_CheckRange(
        L, arg, SAVEGAME_SLOT_POOL_NUMBER_OF, "unknown save pool");
}

// trxc.savegame.slot_count(pool) -> int
static int M_L_SavegameSlotCount(lua_State *const L)
{
    const SAVEGAME_SLOT_POOL pool = M_CheckPool(L, 1);
    if (pool == SAVEGAME_SLOT_POOL_QUICK) {
        lua_pushinteger(L, Savegame_GetQuickVisualCount());
    } else {
        lua_pushinteger(L, Savegame_GetSlotCount(pool));
    }
    return 1;
}

// trxc.savegame.is_free(index, pool) -> bool
static int M_L_SavegameIsFree(lua_State *const L)
{
    const int32_t index = luaL_checkinteger(L, 1);
    const SAVEGAME_SLOT_POOL pool = M_CheckPool(L, 2);
    lua_pushboolean(L, Savegame_IsSlotFree(M_ResolveSlot(index, pool)));
    return 1;
}

// trxc.savegame.load(index, pool)
static int M_L_SavegameLoad(lua_State *const L)
{
    const int32_t index = luaL_checkinteger(L, 1);
    const SAVEGAME_SLOT_POOL pool = M_CheckPool(L, 2);
    GF_OverrideCommand((GF_COMMAND) {
        .action = GF_START_SAVED_GAME,
        .param = Savegame_SlotToParam(M_ResolveSlot(index, pool)),
    });
    return 0;
}

// trxc.savegame.save(index, pool) -> bool
static int M_L_SavegameSave(lua_State *const L)
{
    const SAVEGAME_SLOT_POOL pool = M_CheckPool(L, 2);

    SAVEGAME_SLOT_REF slot;
    if (pool == SAVEGAME_SLOT_POOL_QUICK && lua_isnoneornil(L, 1)) {
        slot = Savegame_GetNextQuickSlot();
        if (!Savegame_IsValidSlotRef(slot)) {
            lua_pushboolean(L, false);
            return 1;
        }
    } else {
        slot = M_ResolveSlot(luaL_checkinteger(L, 1), pool);
    }

    lua_pushboolean(L, Savegame_Save(slot));
    return 1;
}

static const luaL_Reg m_Module[] = {
    { "slot_count", M_L_SavegameSlotCount },
    { "is_free", M_L_SavegameIsFree },
    { "load", M_L_SavegameLoad },
    { "save", M_L_SavegameSave },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_RegisterModule(L, "savegame", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
