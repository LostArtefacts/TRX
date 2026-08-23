#include <trx/game/game_flow.h>
#include <trx/game/gun.h>
#include <trx/game/gun/common.h>
#include <trx/game/gun/registry.h>
#include <trx/game/inventory.h>
#include <trx/game/inventory_ring/types.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/field.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/struct.h>
#include <trx/game/lua/utils.h>
#include <trx/game/savegame.h>

#include <lauxlib.h>

// A level keeps an inventory for Lara's return, and it lives in the game flow
// for the whole session, so what addresses it never goes stale. The ref carries
// the table in the high half and the level's number in the low, the way a level
// handle does - the same packing trx.stats uses.
// The live inventory is the one Lara is carrying; anything else is a level's,
// packed as its table and number.
#define M_LIVE_INVENTORY (-1)
#define M_PACK_LEVEL(table, num) (((table) << 16) | ((num) & 0xffff))
#define M_LEVEL_TABLE(packed) (((packed) >> 16) & 0xff)
#define M_LEVEL_NUM(packed) ((packed) & 0xffff)

// An entry stands for one kind of thing Lara carries, and is addressed by the
// icon it goes into rather than by where it sits: what she has of something
// outlives the position it happens to be drawn at.

// The count is the inventory's, and the view is a copy of it, so a write goes
// back to where it came from rather than into the copy.
static const char *M_SetEntryCount(void *const self, const TRX_VALUE *const in)
{
    const INVENTORY_ENTRY *const entry = self;
    if (in->as_int < 0) {
        return "a count cannot be negative";
    }
    Inv_SetItemCount(entry->object_id, MIN(in->as_int, MAX_QTY));
    return nullptr;
}

// clang-format off
static const FIELD_DESC m_EntryFields[] = {
    FIELD_RO(INVENTORY_ENTRY, object_id),
    FIELD_SET(INVENTORY_ENTRY, qty, M_SetEntryCount),
};
// clang-format on

TYPE_DEFINE(INVENTORY_ENTRY, m_EntryFields)

// The entry a handle stands for, refilled every time it is read: it holds what
// she had at the moment it was asked for, and she goes on picking things up.
static void *M_ResolveEntry(const LUA_STRUCT_REF *const ref)
{
    static INVENTORY_ENTRY entry;
    const OBJECT_ID object_id = (OBJECT_ID)ref->handle.id;
    const int32_t qty = Inv_GetItemCount(object_id);
    if (qty <= 0) {
        return nullptr;
    }
    entry = (INVENTORY_ENTRY) { .object_id = object_id, .qty = qty };
    return &entry;
}

static void M_PushEntry(lua_State *const L, const OBJECT_ID object_id)
{
    LUA_Struct_Push(
        L, &TYPE_INVENTORY_ENTRY, M_ResolveEntry,
        (TRX_HANDLE) { .id = object_id });
}

static void *M_ResolveInventory(const LUA_STRUCT_REF *const ref)
{
    if (ref->handle.id == M_LIVE_INVENTORY) {
        return Inv_GetState();
    }
    const GF_LEVEL *const level = GF_GetLevelByOrdinalNumber(
        (GF_LEVEL_TABLE_TYPE)M_LEVEL_TABLE(ref->handle.id),
        M_LEVEL_NUM(ref->handle.id));
    RESUME_INFO *const resume = SG_Resume_GetEntry(level);
    return resume == nullptr ? nullptr : &resume->inv;
}

// What an inventory holds is reached a thing at a time, so it has no members a
// script reads off it.
TYPE_DEFINE_METHODS_ONLY(INVENTORY_STATE)

// The inventory takes either a pickup or the icon it goes into: the engine
// maps one to the other, so a script may name whichever it has. Either way it
// has to be an object the engine knows, since Inv_AddItem reaches the object
// table with it.

static int32_t M_GetCount(lua_State *const L, const int arg)
{
    const lua_Integer count = luaL_optinteger(L, arg, 1);
    if (count < 1) {
        luaL_argerror(L, arg, "count must be 1 or more");
    }
    return (int32_t)MIN(count, MAX_QTY);
}

static LARA_GUN_TYPE M_GetWeapon(lua_State *const L, const int arg)
{
    const lua_Integer gun_type = luaL_checkinteger(L, arg);
    if (gun_type <= LGT_UNARMED || !Gun_Registry_IsValidType(gun_type)) {
        luaL_argerror(L, arg, "not a weapon");
    }
    return (LARA_GUN_TYPE)gun_type;
}

static INVENTORY_STATE *M_CheckInventory(lua_State *const L, const int arg)
{
    const LUA_STRUCT_REF *const ref =
        LUA_Struct_CheckRef(L, arg, &TYPE_INVENTORY_STATE);
    INVENTORY_STATE *const state = M_ResolveInventory(ref);
    if (state == nullptr) {
        luaL_argerror(L, arg, "stale INVENTORY_STATE handle");
    }
    return state;
}

// Whether this is the inventory Lara is carrying, which is the only one with a
// ring to redraw and meshes to change.
static bool M_IsLive(const INVENTORY_STATE *const state)
{
    return state == Inv_GetState();
}

// inventory:count(object) -> int
static int M_L_InvCount(lua_State *const L)
{
    const INVENTORY_STATE *const state = M_CheckInventory(L, 1);
    lua_pushinteger(L, Inv_State_GetCount(state, LUA_CheckObjectID(L, 2)));
    return 1;
}

// inventory:set_count(object, count)
static int M_L_InvSetCount(lua_State *const L)
{
    INVENTORY_STATE *const state = M_CheckInventory(L, 1);
    const OBJECT_ID object_id = LUA_CheckObjectID(L, 2);
    const lua_Integer count = luaL_checkinteger(L, 3);
    if (count < 0) {
        luaL_argerror(L, 3, "count must be 0 or more");
    }
    if (M_IsLive(state)) {
        Inv_SetItemCount(object_id, (int32_t)MIN(count, MAX_QTY));
    } else {
        Inv_State_SetCount(state, object_id, (int32_t)MIN(count, MAX_QTY));
    }
    return 0;
}

// inventory:has(object) -> bool
static int M_L_InvHas(lua_State *const L)
{
    const INVENTORY_STATE *const state = M_CheckInventory(L, 1);
    lua_pushboolean(L, Inv_State_Has(state, LUA_CheckObjectID(L, 2)));
    return 1;
}

// inventory:give(object, [count]) -> int
//
// Lara's own inventory takes it the way walking over it would, so a weapon
// brings its rounds and the level's guns turn into ammunition for it. A level's
// is what she will arrive with, and the count is the whole of it.
static int M_L_InvGive(lua_State *const L)
{
    INVENTORY_STATE *const state = M_CheckInventory(L, 1);
    const OBJECT_ID object_id = LUA_CheckObjectID(L, 2);
    const int32_t count = M_GetCount(L, 3);
    if (M_IsLive(state)) {
        int32_t given = 0;
        for (int32_t i = 0; i < count && Inv_AddItem(object_id); i++) {
            given++;
        }
        lua_pushinteger(L, given);
        return 1;
    }

    const LARA_GUN_TYPE gun_type = Gun_GetType(Inv_GetItemPickup(object_id));
    if (gun_type != LGT_UNARMED && !Inv_State_Has(state, object_id)) {
        Inv_State_AddAmmo(state, gun_type, Gun_GetInitialRounds(gun_type));
    }
    Inv_State_AddCount(state, object_id, count);
    lua_pushinteger(L, count);
    return 1;
}

// inventory:take(object, [count]) -> int
static int M_L_InvTake(lua_State *const L)
{
    INVENTORY_STATE *const state = M_CheckInventory(L, 1);
    const OBJECT_ID object_id = LUA_CheckObjectID(L, 2);
    const int32_t count = M_GetCount(L, 3);
    if (M_IsLive(state)) {
        int32_t taken = 0;
        for (int32_t i = 0; i < count && Inv_RemoveItem(object_id); i++) {
            taken++;
        }
        lua_pushinteger(L, taken);
        return 1;
    }

    const int32_t held = Inv_State_GetCount(state, object_id);
    const int32_t taken = MIN(count, held);
    Inv_State_SetCount(state, object_id, held - taken);
    lua_pushinteger(L, taken);
    return 1;
}

// inventory:shots(weapon) -> int
static int M_L_InvShots(lua_State *const L)
{
    const INVENTORY_STATE *const state = M_CheckInventory(L, 1);
    const LARA_GUN_TYPE gun_type = M_GetWeapon(L, 2);
    lua_pushinteger(
        L, Inv_State_GetAmmo(state, gun_type) / Gun_GetRoundsPerShot(gun_type));
    return 1;
}

// inventory:set_shots(weapon, count)
static int M_L_InvSetShots(lua_State *const L)
{
    INVENTORY_STATE *const state = M_CheckInventory(L, 1);
    const LARA_GUN_TYPE gun_type = M_GetWeapon(L, 2);
    const lua_Integer shots = luaL_checkinteger(L, 3);
    if (shots < 0) {
        luaL_argerror(L, 3, "shots must be 0 or more");
    }
    const int32_t rounds =
        (int32_t)MIN(shots, MAX_QTY) * Gun_GetRoundsPerShot(gun_type);
    if (M_IsLive(state)) {
        Inv_SetAmmo(gun_type, rounds);
    } else {
        Inv_State_SetAmmo(state, gun_type, rounds);
    }
    return 0;
}

// inventory:has_weapon(weapon) -> bool
static int M_L_InvHasWeapon(lua_State *const L)
{
    const INVENTORY_STATE *const state = M_CheckInventory(L, 1);
    lua_pushboolean(
        L, Inv_State_Has(state, Gun_GetGunObject(M_GetWeapon(L, 2))));
    return 1;
}

// inventory:entry_at(n) -> INVENTORY_ENTRY handle or nil
static int M_L_InvEntryAt(lua_State *const L)
{
    const INVENTORY_STATE *const state = M_CheckInventory(L, 1);
    INVENTORY_ENTRY entries[INV_MAX_ENTRIES];
    const int32_t count =
        Inv_State_GetDrawnEntries(state, entries, INV_MAX_ENTRIES);
    int32_t idx;
    if (!LUA_CheckBoundedInt(L, 2, 1, count, &idx)) {
        lua_pushnil(L);
        return 1;
    }
    M_PushEntry(L, entries[idx - 1].object_id);
    return 1;
}

// inventory:entry(object) -> INVENTORY_ENTRY handle or nil
static int M_L_InvEntry(lua_State *const L)
{
    const INVENTORY_STATE *const state = M_CheckInventory(L, 1);
    const OBJECT_ID object_id = LUA_CheckObjectID(L, 2);
    if (!Inv_State_Has(state, object_id)) {
        lua_pushnil(L);
        return 1;
    }
    M_PushEntry(L, Inv_GetItemOption(object_id));
    return 1;
}

// inventory:entry_count() -> int
static int M_L_InvEntryCount(lua_State *const L)
{
    const INVENTORY_STATE *const state = M_CheckInventory(L, 1);
    INVENTORY_ENTRY entries[INV_MAX_ENTRIES];
    lua_pushinteger(
        L, Inv_State_GetDrawnEntries(state, entries, INV_MAX_ENTRIES));
    return 1;
}

// inventory:icon_of(object) -> object id or nil
static int M_L_InvIconOf(lua_State *const L)
{
    M_CheckInventory(L, 1);
    const OBJECT_ID icon_id = Inv_GetItemOption(LUA_CheckObjectID(L, 2));
    if (icon_id == NO_OBJECT) {
        lua_pushnil(L);
    } else {
        lua_pushinteger(L, icon_id);
    }
    return 1;
}

// inventory:can_add(object) -> bool
//
// The one question only the level being played can answer: whether it carries
// the model the ring draws, which is what lets a cheat hand a weapon over.
static int M_L_InvCanAdd(lua_State *const L)
{
    M_CheckInventory(L, 1);
    lua_pushboolean(L, Inv_CanAddItem(LUA_CheckObjectID(L, 2)));
    return 1;
}

// trxc.inventory.get(level_num) -> INVENTORY_STATE handle or nil
static int M_L_InvGetLevel(lua_State *const L)
{
    int32_t num;
    if (!LUA_CheckBoundedInt(L, 1, 0, INT32_MAX, &num)) {
        lua_pushnil(L);
        return 1;
    }
    const GF_LEVEL *const level = GF_GetLevelByOrdinalNumber(GFLT_MAIN, num);
    if (level == nullptr || SG_Resume_GetEntry(level) == nullptr) {
        lua_pushnil(L);
        return 1;
    }
    LUA_Struct_Push(
        L, &TYPE_INVENTORY_STATE, M_ResolveInventory,
        (TRX_HANDLE) { .id = M_PACK_LEVEL(GFLT_MAIN, num) });
    return 1;
}

// trxc.inventory.get_current() -> INVENTORY_STATE handle
static int M_L_InvGetCurrent(lua_State *const L)
{
    LUA_Struct_Push(
        L, &TYPE_INVENTORY_STATE, M_ResolveInventory,
        (TRX_HANDLE) { .id = M_LIVE_INVENTORY });
    return 1;
}

// A round is what one shot at a target spends, and the shotgun spends six of
// them at once. What a script counts in is shots; the rounds behind them are
// there for a save that holds an odd number of them.
static int32_t M_GetRounds(lua_State *const L, const int arg)
{
    const lua_Integer count = luaL_checkinteger(L, arg);
    if (count < 0) {
        luaL_argerror(L, arg, "ammo must be 0 or more");
    }
    return (int32_t)MIN(count, MAX_QTY);
}

static const luaL_Reg m_InventoryMethods[] = {
    { "count", M_L_InvCount },
    { "set_count", M_L_InvSetCount },
    { "has", M_L_InvHas },
    { "give", M_L_InvGive },
    { "take", M_L_InvTake },
    { "shots", M_L_InvShots },
    { "set_shots", M_L_InvSetShots },
    { "has_weapon", M_L_InvHasWeapon },
    { "entry", M_L_InvEntry },
    { "entry_at", M_L_InvEntryAt },
    { "entry_count", M_L_InvEntryCount },
    { "icon_of", M_L_InvIconOf },
    { "can_add", M_L_InvCanAdd },
    { nullptr, nullptr },
};

static const luaL_Reg m_Module[] = {
    { "get_current", M_L_InvGetCurrent },
    { "get", M_L_InvGetLevel },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_Struct_Register(L, &TYPE_INVENTORY_ENTRY, nullptr);
    LUA_Struct_Register(L, &TYPE_INVENTORY_STATE, m_InventoryMethods);
    LUA_RegisterModule(L, "inventory", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
