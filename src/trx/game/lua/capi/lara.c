#include <trx/game/gun.h>
#include <trx/game/inventory.h>
#include <trx/game/lara.h>
#include <trx/game/lara/skin/storage.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/struct.h>
#include <trx/game/lua/utils.h>

#include <lauxlib.h>

extern const TYPE_DESC TYPE_LARA_INFO;

static void *M_Resolve(const LUA_STRUCT_REF *const ref)
{
    return Lara_GetLaraInfo();
}

// trxc.lara.state() -> LARA_INFO handle
static int M_L_LaraState(lua_State *const L)
{
    LUA_Struct_Push(L, &TYPE_LARA_INFO, M_Resolve, 0, 0);
    return 1;
}

// item_num = trxc.lara.get_item()
static int M_L_GetLaraItem(lua_State *const L)
{
    const ITEM *const item = Lara_GetItem();
    int result = 0;
    if (item != nullptr) {
        result = Item_GetIndex(item) + 1;
    }
    if (result == 0) {
        lua_pushnil(L);
    } else {
        lua_pushinteger(L, result);
    }
    return 1;
}

// item_num = trxc.lara.get_target()
static int M_L_GetLaraTarget(lua_State *const L)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->target == nullptr) {
        lua_pushnil(L);
    } else {
        lua_pushinteger(L, Item_GetIndex(lara->target) + 1);
    }
    return 1;
}

// trxc.lara.get_exposure_bar() → int
static int M_L_LaraGetExposureBar(lua_State *const L)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lua_pushinteger(L, lara->exposure_timer);
    return 1;
}

// trxc.lara.set_exposure_bar(timer)
static int M_L_LaraSetExposureBar(lua_State *const L)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->exposure_timer = luaL_checkinteger(L, 1);
    return 0;
}

// trxc.lara.get_air_bar() → int
static int M_L_LaraGetAirBar(lua_State *const L)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lua_pushinteger(L, lara->air);
    return 1;
}

// trxc.lara.set_air_bar(timer)
static int M_L_LaraSetAirBar(lua_State *const L)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->air = luaL_checkinteger(L, 1);
    return 0;
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
    const bool visible = lua_toboolean(L, 1) != 0;
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

// trxc.lara.get_equipped_gun() → int
static int M_L_LaraGetEquippedGun(lua_State *const L)
{
    lua_pushinteger(L, Lara_GetLaraInfo()->gun_type);
    return 1;
}

static void M_Create(lua_State *const L)
{
    LUA_Struct_Register(
        L, &TYPE_LARA_INFO, (const luaL_Reg[]) { { nullptr, nullptr } });

    lua_getglobal(L, "trxc");
    lua_newtable(L);

    lua_pushcfunction(L, M_L_GetLaraItem);
    lua_setfield(L, -2, "get_item");
    lua_pushcfunction(L, M_L_GetLaraTarget);
    lua_setfield(L, -2, "get_target");
    lua_pushcfunction(L, M_L_LaraGetExposureBar);
    lua_setfield(L, -2, "get_exposure_bar");
    lua_pushcfunction(L, M_L_LaraSetExposureBar);
    lua_setfield(L, -2, "set_exposure_bar");
    lua_pushcfunction(L, M_L_LaraGetAirBar);
    lua_setfield(L, -2, "get_air_bar");
    lua_pushcfunction(L, M_L_LaraSetAirBar);
    lua_setfield(L, -2, "set_air_bar");
    lua_pushcfunction(L, M_L_LaraGetOutfit);
    lua_setfield(L, -2, "get_outfit");
    lua_pushcfunction(L, M_L_LaraSetOutfit);
    lua_setfield(L, -2, "set_outfit");
    lua_pushcfunction(L, M_L_LaraSetExtraEquipment);
    lua_setfield(L, -2, "set_extra_equipment");
    lua_pushcfunction(L, M_L_LaraClearEquipment);
    lua_setfield(L, -2, "clear_equipment");
    lua_pushcfunction(L, M_L_LaraAreHolstersVisible);
    lua_setfield(L, -2, "are_holsters_visible");
    lua_pushcfunction(L, M_L_LaraSetHolstersVisible);
    lua_setfield(L, -2, "set_holsters_visible");
    lua_pushcfunction(L, M_L_LaraHasPistolWeapon);
    lua_setfield(L, -2, "has_pistol_weapon");
    lua_pushcfunction(L, M_L_LaraGetExtraAnim);
    lua_setfield(L, -2, "get_extra_anim");
    lua_pushcfunction(L, M_L_LaraGetEquippedGun);
    lua_setfield(L, -2, "get_equipped_gun");
    lua_pushcfunction(L, M_L_LaraState);
    lua_setfield(L, -2, "state");

    lua_setfield(L, -2, "lara");
    lua_pop(L, 1);
}

REGISTER_LUA_CAPI(.create = M_Create)
