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
    FIELD_RO(LARA_INFO, killed_loyal_item),

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
            && Inv_RequestItem(Gun_GetGunObject(i))) {
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

static const luaL_Reg m_Module[] = {
    { "get_item", M_L_LaraGetItem },
    { "get_target", M_L_LaraGetTarget },
    { "get_outfit", M_L_LaraGetOutfit },
    { "set_outfit", M_L_LaraSetOutfit },
    { "set_extra_equipment", M_L_LaraSetExtraEquipment },
    { "clear_equipment", M_L_LaraClearEquipment },
    { "cure_poison", M_L_LaraCurePoison },
    { "extinguish", M_L_LaraExtinguish },
    { "is_flying", M_L_LaraIsFlying },
    { "set_flying", M_L_LaraSetFlying },
    { "are_holsters_visible", M_L_LaraAreHolstersVisible },
    { "set_holsters_visible", M_L_LaraSetHolstersVisible },
    { "has_pistol_weapon", M_L_LaraHasPistolWeapon },
    { "get_extra_anim", M_L_LaraGetExtraAnim },
    { "state", M_L_LaraState },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_Struct_Register(L, &TYPE_LARA_INFO, nullptr);
    LUA_RegisterModule(L, "lara", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
