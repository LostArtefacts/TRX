#include <trx/game/gun.h>
#include <trx/game/inventory.h>
#include <trx/game/items/const.h>
#include <trx/game/lara.h>
#include <trx/game/lara/cheat.h>
#include <trx/game/lara/misc.h>
#include <trx/game/lara/poison.h>
#include <trx/game/lara/skin/storage.h>
#include <trx/game/lara/types.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/field.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/struct.h>
#include <trx/game/lua/utils.h>
#include <trx/game/rooms.h>

#include <lauxlib.h>

// Lara's position, room and hit points are not here. She is an item like any
// other, and those live on it - see trx.lara.item.

// Setting the flag alone would not light her, so is_burning goes through the
// same catch-fire and extinguish paths a flame trap does.
static bool M_GetBurn(const void *const self, TRX_VALUE *const out)
{
    const LARA_INFO *const lara = self;
    *out = (TRX_VALUE) { .type = TVT_BOOL, .as_bool = lara->burn };
    return true;
}

static const char *M_SetBurn(void *const self, const TRX_VALUE *const in)
{
    if (in->as_bool) {
        Lara_CatchFire();
    } else {
        Lara_Extinguish();
    }
    return nullptr;
}

// clang-format off
static const FIELD_DESC m_Fields[] = {
    // condition
    FIELD(LARA_INFO, air),
    FIELD(LARA_INFO, exposure_timer),
    FIELD(LARA_INFO, poison.value),
    FIELD(LARA_INFO, poison.target),
    FIELD(LARA_INFO, electric),
    FIELD_FN("burn", TVT_BOOL, M_GetBurn, M_SetBurn),

    // what she is doing
    FIELD_RO(LARA_INFO, water_status),
    FIELD_RO(LARA_INFO, gun_status),
    FIELD_RO(LARA_INFO, gun_type),
    FIELD_RO(LARA_INFO, request_gun_type),
    FIELD_RO(LARA_INFO, is_crouched),
    FIELD_RO(LARA_INFO, climb_status),
    FIELD_RO(LARA_INFO, extra_anim),

    // timers
    FIELD_RO(LARA_INFO, dive_timer),
    FIELD_RO(LARA_INFO, death_timer),
    FIELD(LARA_INFO, sprint_timer),
    FIELD_RO(LARA_INFO, hit_direction),
    FIELD_RO(LARA_INFO, hit_frame),
    FIELD_RO(LARA_INFO, pose_count),

    // Deliberately absent: the arms, the LOT, the mesh pointers, the rope and
    // interaction state, and the interpolation snapshots. They are how the
    // engine drives Lara, not a contract, and a script writing them mid-frame
    // would wedge her.
};
// clang-format on

TYPE_DEFINE(LARA_INFO, m_Fields)

static void *M_Resolve(const LUA_STRUCT_REF *const ref)
{
    return Lara_GetLaraInfo();
}

// M_SetEquipment writes through &m_Equipment[mesh] without checking it.
static LARA_MESH M_CheckLaraMesh(lua_State *const L, const int arg)
{
    return (LARA_MESH)LUA_CheckRange(L, arg, LM_NUMBER_OF, "unknown Lara mesh");
}

// Lara_Skin_GetExtraMeshOffset asserts, which traps rather than raises.
static LARA_SKIN_EXTRA_MESH M_CheckExtraMesh(lua_State *const L, const int arg)
{
    return (LARA_SKIN_EXTRA_MESH)LUA_CheckRange(
        L, arg, NUM_EXTRA_MESHES, "unknown extra mesh");
}

// trxc.lara.state() -> LARA_INFO handle
static int M_L_LaraState(lua_State *const L)
{
    LUA_Struct_Push(L, &TYPE_LARA_INFO, M_Resolve, (TRX_HANDLE) { .id = 0 });
    return 1;
}

// item_num = trxc.lara.get_item()
static int M_L_LaraGetItem(lua_State *const L)
{
    const ITEM *const item = Lara_GetItem();
    LUA_PushOptIndex(
        L, item != nullptr ? Item_GetIndex(item) : NO_ITEM, NO_ITEM);
    return 1;
}

// item_num = trxc.lara.get_target()
static int M_L_LaraGetTarget(lua_State *const L)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    LUA_PushOptIndex(
        L, lara->target != nullptr ? Item_GetIndex(lara->target) : NO_ITEM,
        NO_ITEM);
    return 1;
}

// trxc.lara.get_outfit() → string
static int M_L_LaraGetOutfit(lua_State *const L)
{
    const int32_t outfit_idx = Lara_Skin_GetType();
    const char *const outfit_name = Lara_Skin_GetOutfitName(outfit_idx);
    if (outfit_name == nullptr) {
        lua_pushnil(L);
    } else {
        lua_pushstring(L, outfit_name);
    }
    return 1;
}

// trxc.lara.set_outfit(outfit_name)
static int M_L_LaraSetOutfit(lua_State *const L)
{
    const char *const outfit_name = luaL_checkstring(L, 1);
    const int32_t outfit_idx = Lara_Skin_FindOutfitByName(outfit_name);
    if (!Lara_Skin_IsOutfitAvailable(outfit_idx)) {
        return luaL_error(L, "unknown Lara outfit: %s", outfit_name);
    }
    Lara_Skin_SetType(outfit_idx);
    return 0;
}

// trxc.lara.set_extra_equipment(lara_mesh, extra_mesh)
static int M_L_LaraSetExtraEquipment(lua_State *const L)
{
    const LARA_MESH lara_mesh = M_CheckLaraMesh(L, 1);
    const LARA_SKIN_EXTRA_MESH extra_mesh = M_CheckExtraMesh(L, 2);
    Lara_Skin_SetExtraEquipment(lara_mesh, extra_mesh);
    return 0;
}

// trxc.lara.clear_equipment(lara_mesh)
static int M_L_LaraClearEquipment(lua_State *const L)
{
    const LARA_MESH lara_mesh = M_CheckLaraMesh(L, 1);
    Lara_Skin_ClearEquipment(lara_mesh);
    return 0;
}

// trxc.lara.are_holsters_visible() → bool
static int M_L_LaraAreHolstersVisible(lua_State *const L)
{
    lua_pushboolean(L, Lara_Skin_AreHolstersVisible());
    return 1;
}

// trxc.lara.set_holsters_visible(visible)
static int M_L_LaraSetHolstersVisible(lua_State *const L)
{
    const bool visible = lua_toboolean(L, 1);
    Lara_Skin_SetHolstersVisible(visible);
    return 0;
}

// trxc.lara.has_pistol_weapon() → bool
static int M_L_LaraHasPistolWeapon(lua_State *const L)
{
    bool has_pistol = false;
    for (int32_t i = 0; i < NUM_WEAPONS; i++) {
        const WEAPON_INFO *const weapon = &g_Weapons[i];
        if ((weapon->type == WEAPON_TYPE_DUAL_PISTOLS
             || weapon->type == WEAPON_TYPE_SINGLE_PISTOL)
            && Inv_GetItemCount(Gun_GetGunObject(i))) {
            has_pistol = true;
            break;
        }
    }
    lua_pushboolean(L, has_pistol);
    return 1;
}

// trxc.lara.get_extra_anim() → int
static int M_L_LaraGetExtraAnim(lua_State *const L)
{
    if (Lara_GetLaraInfo()->extra_anim) {
        lua_pushinteger(
            L, Item_GetRelativeObjAnim(Lara_GetItem(), O_LARA_EXTRA));
    } else {
        lua_pushinteger(L, NO_ANIM);
    }
    return 1;
}

// trxc.lara.cure_poison()
static int M_L_LaraCurePoison(lua_State *const L)
{
    Lara_Poison_Cure();
    return 0;
}

// trxc.lara.extinguish()
static int M_L_LaraExtinguish(lua_State *const L)
{
    Lara_Extinguish();
    return 0;
}

// trxc.lara.dry()
static int M_L_LaraDry(lua_State *const L)
{
    Lara_Dry();
    return 0;
}

// trxc.lara.is_wet() → bool
static int M_L_LaraIsWet(lua_State *const L)
{
    lua_pushboolean(L, Lara_IsWet());
    return 1;
}

// trxc.lara.is_flying() -> bool
static int M_L_LaraIsFlying(lua_State *const L)
{
    lua_pushboolean(L, Lara_GetLaraInfo()->water_status == LWS_CHEAT);
    return 1;
}

// trxc.lara.set_flying(bool)
static int M_L_LaraSetFlying(lua_State *const L)
{
    if (lua_toboolean(L, 1)) {
        Lara_Cheat_EnterFlyMode();
    } else {
        Lara_Cheat_ExitFlyMode();
    }
    return 0;
}

// trxc.lara.teleport({x,y,z}, room_num) -> bool
static int M_L_LaraTeleport(lua_State *const L)
{
    const XYZ_32 pos = LUA_CheckXYZ(L, 1);

    // Without a room, Lara_Cheat_Teleport finds one from the position.
    int16_t room_num = NO_ROOM;
    if (!lua_isnoneornil(L, 2)) {
        const lua_Integer room_arg = luaL_checkinteger(L, 2);
        luaL_argcheck(
            L, room_arg >= 0 && room_arg <= Room_GetCount() - 1, 2,
            "unknown room");
        room_num = (int16_t)room_arg;
    }

    lua_pushboolean(L, Lara_Cheat_Teleport(pos, room_num));
    return 1;
}

// The backpack takes either a pickup or the inventory icon it goes into: the
// engine maps one to the other, so a script may name whichever it has. Either
// way it has to be an object the engine knows, since Inv_AddItem reaches the
// object table with it.

static int32_t M_GetInvCount(lua_State *const L, const int arg)
{
    const lua_Integer count = luaL_optinteger(L, arg, 1);
    if (count < 1) {
        luaL_argerror(L, arg, "count must be 1 or more");
    }
    return (int32_t)MIN(count, MAX_QTY);
}

// trxc.lara.inventory.add(object, [count]) -> int
static int M_L_LaraInvAdd(lua_State *const L)
{
    const OBJECT_ID object_id = LUA_CheckObjectID(L, 1);
    const int32_t count = M_GetInvCount(L, 2);
    int32_t added = 0;
    for (int32_t i = 0; i < count && Inv_AddItem(object_id); i++) {
        added++;
    }
    lua_pushinteger(L, added);
    return 1;
}

// trxc.lara.inventory.remove(object, [count]) -> int
static int M_L_LaraInvRemove(lua_State *const L)
{
    const OBJECT_ID object_id = LUA_CheckObjectID(L, 1);
    const int32_t count = M_GetInvCount(L, 2);
    int32_t removed = 0;
    for (int32_t i = 0; i < count && Inv_RemoveItem(object_id); i++) {
        removed++;
    }
    lua_pushinteger(L, removed);
    return 1;
}

// trxc.lara.inventory.count(object) -> int
static int M_L_LaraInvCount(lua_State *const L)
{
    lua_pushinteger(L, Inv_GetItemCount(LUA_CheckObjectID(L, 1)));
    return 1;
}

// trxc.lara.inventory.entry_of(object) -> object id
static int M_L_LaraInvEntryOf(lua_State *const L)
{
    lua_pushinteger(L, Inv_GetItemOption(LUA_CheckObjectID(L, 1)));
    return 1;
}

// trxc.lara.inventory.can_add(object) -> bool
//
// The icon is what has to be there: a level with no shotgun lying in it still
// carries the model the ring draws, which is what lets the cheat hand one over.
static int M_L_LaraInvCanAdd(lua_State *const L)
{
    lua_pushboolean(L, Inv_CanAddItem(LUA_CheckObjectID(L, 1)));
    return 1;
}

static LARA_GUN_TYPE M_GetWeapon(lua_State *const L, const int arg)
{
    const lua_Integer gun_type = luaL_checkinteger(L, arg);
    if (gun_type <= LGT_UNARMED || gun_type >= NUM_WEAPONS) {
        luaL_argerror(L, arg, "not a weapon");
    }
    return (LARA_GUN_TYPE)gun_type;
}

// trxc.lara.weapons.is_available(weapon) -> bool
static int M_L_LaraWeaponIsAvailable(lua_State *const L)
{
    lua_pushboolean(L, g_Weapons[M_GetWeapon(L, 1)].is_available);
    return 1;
}

// trxc.lara.weapons.get_object(weapon) -> object id or nil
static int M_L_LaraWeaponGetObject(lua_State *const L)
{
    const OBJECT_ID object_id = Gun_GetGunObject(M_GetWeapon(L, 1));
    if (object_id == NO_OBJECT) {
        lua_pushnil(L);
    } else {
        lua_pushinteger(L, object_id);
    }
    return 1;
}

// trxc.lara.weapons.get_ammo(weapon) -> int
static int M_L_LaraWeaponGetAmmo(lua_State *const L)
{
    lua_pushinteger(L, Inv_GetAmmo(M_GetWeapon(L, 1)));
    return 1;
}

// trxc.lara.weapons.set_ammo(weapon, count)
static int M_L_LaraWeaponSetAmmo(lua_State *const L)
{
    const LARA_GUN_TYPE gun_type = M_GetWeapon(L, 1);
    const lua_Integer count = luaL_checkinteger(L, 2);
    if (count < 0) {
        luaL_argerror(L, 2, "ammo must be 0 or more");
    }
    Inv_SetAmmo(gun_type, (int32_t)MIN(count, MAX_QTY));
    return 0;
}

static const luaL_Reg m_Inventory[] = {
    { "add", M_L_LaraInvAdd },          { "remove", M_L_LaraInvRemove },
    { "count", M_L_LaraInvCount },      { "can_add", M_L_LaraInvCanAdd },
    { "entry_of", M_L_LaraInvEntryOf }, { nullptr, nullptr },
};

static const luaL_Reg m_Weapons[] = {
    { "is_available", M_L_LaraWeaponIsAvailable },
    { "get_object", M_L_LaraWeaponGetObject },
    { "get_ammo", M_L_LaraWeaponGetAmmo },
    { "set_ammo", M_L_LaraWeaponSetAmmo },
    { nullptr, nullptr },
};

static const luaL_Reg m_Module[] = {
    { "get_item", M_L_LaraGetItem },
    { "get_target", M_L_LaraGetTarget },
    { "get_outfit", M_L_LaraGetOutfit },
    { "set_outfit", M_L_LaraSetOutfit },
    { "set_extra_equipment", M_L_LaraSetExtraEquipment },
    { "clear_equipment", M_L_LaraClearEquipment },
    { "cure_poison", M_L_LaraCurePoison },
    { "extinguish", M_L_LaraExtinguish },
    { "dry", M_L_LaraDry },
    { "is_wet", M_L_LaraIsWet },
    { "is_flying", M_L_LaraIsFlying },
    { "set_flying", M_L_LaraSetFlying },
    { "are_holsters_visible", M_L_LaraAreHolstersVisible },
    { "set_holsters_visible", M_L_LaraSetHolstersVisible },
    { "has_pistol_weapon", M_L_LaraHasPistolWeapon },
    { "get_extra_anim", M_L_LaraGetExtraAnim },
    { "teleport", M_L_LaraTeleport },
    { "state", M_L_LaraState },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_Struct_Register(L, &TYPE_LARA_INFO, nullptr);
    LUA_RegisterModule(L, "lara", m_Module);

    // What Lara carries, and what she carries it in, are groups of their own on
    // the module.
    LUA_GetModule(L, "lara");
    lua_newtable(L);
    luaL_setfuncs(L, m_Inventory, 0);
    lua_setfield(L, -2, "inventory");
    lua_newtable(L);
    luaL_setfuncs(L, m_Weapons, 0);
    lua_setfield(L, -2, "weapons");
    lua_pop(L, 1);
}

REGISTER_LUA_CAPI(.create = M_Create)
