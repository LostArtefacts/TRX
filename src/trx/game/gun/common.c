#include <trx/config.h>
#include <trx/core/colors.h>
#include <trx/debug.h>
#include <trx/game/const.h>
#include <trx/game/game.h>
#include <trx/game/gun.h>
#include <trx/game/gun/registry.h>
#include <trx/game/lara.h>
#include <trx/game/output.h>
#include <trx/version.h>

// What a weapon is made of outside the numbers weapons.json5 carries: the
// pickup lying in the world, the box of ammunition for it, and the animation
// object Lara holds it with. A weapon a game has no object for reads
// NO_OBJECT, as do the flare and the skidoo, which are held like weapons
// without being ones.

// Which weapon Lara wears where, when she carries more than one that could
// hang there. The weapon type says which of the two slots a weapon can use -
// the rifles go on her back and the rest into her holsters - but not which of

static LARA_GUN_TYPE M_GetFirstCarried(
    const INVENTORY_STATE *const inv, const STOW_PLACE place)
{
    for (int32_t order = 1; order <= Gun_Registry_GetCount(); order++) {
        for (int32_t i = 0; i < Gun_Registry_GetCount(); i++) {
            const WEAPON_INFO *const weapon = Gun_Registry_GetByIndex(i);
            const LARA_GUN_TYPE gun_type = weapon->gun_type;
            if (weapon->stow_place != place || weapon->stow_order != order) {
                continue;
            }
            if (Inv_State_Has(inv, Gun_GetGunObject(gun_type))) {
                return gun_type;
            }
        }
    }
    return LGT_UNARMED;
}

static bool M_IsGunType(
    const LARA_GUN_TYPE gun_type, const WEAPON_TYPE weapon_type)
{
    return Gun_Registry_Get(gun_type)->type == weapon_type;
}

// weapons.json5 counts in shots, which is what the player's counter shows. The
// engine spends a round at a time and the shotgun spends six of them on one
// shot, so the quantities there are scaled into what it spends.
static int32_t M_GetRounds(const LARA_GUN_TYPE gun_type, const int32_t box_qty)
{
    return MAX(1, box_qty) * Gun_GetRoundsPerShot(gun_type);
}

void Gun_AddDynamicLight(void)
{
    if (!g_Config.visuals.enable_gun_lighting) {
        return;
    }

    const ITEM *const lara_item = Lara_GetItem();
    XYZ_32 pos = XYZ_32_OffsetYaw(lara_item->pos, lara_item->rot.y, 1024);
    pos.y -= WALL_L / 2;
    if (g_TRVersion >= 3) {
        Output_AddDynamicLightRGB(pos, 12, (RGB_888) { 192, 144, 0 });
    } else {
        Output_AddDynamicLight(pos, 12, 11);
    }
}

OBJECT_ID Gun_GetLaraAnim(const LARA_GUN_TYPE gun_type)
{
    return Gun_IsDualPistolType(gun_type) ? O_LARA_PISTOLS
                                          : Gun_GetWeaponAnim(gun_type);
}

OBJECT_ID Gun_GetWeaponAnim(const LARA_GUN_TYPE gun_type)
{
    // A legacy save can name a gun type the engine no longer knows, and
    // unarmed has no weapon of its own; both hold Lara's plain animations.
    if (gun_type <= LGT_UNARMED) {
        return O_LARA;
    }
    return Gun_Registry_IsValidType(gun_type)
        ? Gun_Registry_Get(gun_type)->anim_object_id
        : NO_OBJECT;
}

LARA_GUN_TYPE Gun_GetType(const OBJECT_ID obj_id)
{
    if (obj_id != NO_OBJECT) {
        for (int32_t i = 0; i < Gun_Registry_GetCount(); i++) {
            const WEAPON_INFO *const weapon = Gun_Registry_GetByIndex(i);
            if (weapon->gun_object_id == obj_id) {
                return weapon->gun_type;
            }
        }
    }
    return LGT_UNARMED;
}

OBJECT_ID Gun_GetGunObject(const LARA_GUN_TYPE gun_type)
{
    return Gun_Registry_IsValidType(gun_type)
        ? Gun_Registry_Get(gun_type)->gun_object_id
        : NO_OBJECT;
}

OBJECT_ID Gun_GetAmmoObject(const LARA_GUN_TYPE gun_type)
{
    return Gun_Registry_IsValidType(gun_type)
        ? Gun_Registry_Get(gun_type)->ammo_object_id
        : NO_OBJECT;
}

int32_t Gun_GetInitialRounds(const LARA_GUN_TYPE gun_type)
{
    return M_GetRounds(
        gun_type, Gun_Registry_Get(gun_type)->ammo.initial_shots);
}

int32_t Gun_GetRoundsPerBox(const LARA_GUN_TYPE gun_type)
{
    return M_GetRounds(gun_type, Gun_Registry_Get(gun_type)->ammo.box_shots);
}

int32_t Gun_GetAmmoInventoryQuantity(const LARA_GUN_TYPE gun_type)
{
    return Gun_Registry_Get(gun_type)->ammo.box_label_qty;
}

int32_t Gun_GetRoundsPerShot(const LARA_GUN_TYPE gun_type)
{
    const int32_t rounds = Gun_Registry_Get(gun_type)->ammo.rounds_per_shot;
    return rounds > 0 ? rounds : 1;
}

LARA_GUN_TYPE Gun_GetTypeForObject(const OBJECT_ID obj_id)
{
    if (obj_id == NO_OBJECT) {
        return LGT_UNARMED;
    }
    for (int32_t i = 0; i < Gun_Registry_GetCount(); i++) {
        const WEAPON_INFO *const weapon = Gun_Registry_GetByIndex(i);
        if (weapon->gun_object_id == obj_id) {
            return weapon->gun_type;
        }
    }
    return LGT_UNARMED;
}

LARA_GUN_TYPE Gun_GetTypeForInputRole(const INPUT_ROLE role)
{
    for (int32_t i = 0; i < Gun_Registry_GetCount(); i++) {
        const WEAPON_INFO *const weapon = Gun_Registry_GetByIndex(i);
        if (weapon->equip_input_role == role) {
            return weapon->gun_type;
        }
    }
    return LGT_UNARMED;
}

LARA_GUN_TYPE Gun_GetHolsterChoice(const INVENTORY_STATE *const inv)
{
    return M_GetFirstCarried(inv, STOW_PLACE_HOLSTER);
}

LARA_GUN_TYPE Gun_GetBackChoice(const INVENTORY_STATE *const inv)
{
    return M_GetFirstCarried(inv, STOW_PLACE_BACK);
}

bool Gun_HasInfiniteAmmo(const LARA_GUN_TYPE gun_type)
{
    if (Game_IsBonusFlagSet(GBF_NGPLUS)) {
        return true;
    }
    return Gun_Registry_IsValidType(gun_type)
        && Gun_Registry_Get(gun_type)->ammo.infinite;
}

bool Gun_HasRoundsLeft(const LARA_GUN_TYPE gun_type)
{
    return Gun_HasInfiniteAmmo(gun_type) || Inv_GetAmmo(gun_type) > 0;
}

void Gun_SpendRound(const LARA_GUN_TYPE gun_type)
{
    if (!Gun_HasInfiniteAmmo(gun_type)) {
        Inv_AddAmmo(gun_type, -1);
    }
}

bool Gun_HasAvailableMachineGun(void)
{
    for (int32_t i = 0; i < Gun_Registry_GetCount(); i++) {
        const WEAPON_INFO *const info = Gun_Registry_GetByIndex(i);
        if (info->is_machine_gun && info->is_available) {
            return true;
        }
    }
    return false;
}

bool Gun_HasAvailableLauncher(void)
{
    for (int32_t i = 0; i < Gun_Registry_GetCount(); i++) {
        const WEAPON_INFO *const info = Gun_Registry_GetByIndex(i);
        if (info->is_launcher && info->is_available) {
            return true;
        }
    }
    return false;
}

LARA_GUN_TYPE Gun_GetDefaultType(void)
{
    for (int32_t i = 0; i < Gun_Registry_GetCount(); i++) {
        const WEAPON_INFO *const info = Gun_Registry_GetByIndex(i);
        if (info->is_default) {
            return info->gun_type;
        }
    }
    return LGT_UNARMED;
}

bool Gun_IsMachineGunType(const LARA_GUN_TYPE gun_type)
{
    const WEAPON_INFO *const info = Gun_Registry_Get(gun_type);
    return info->is_machine_gun;
}

bool Gun_FiresProjectile(const LARA_GUN_TYPE gun_type)
{
    return Gun_GetProjectileObject(gun_type) != NO_OBJECT;
}

OBJECT_ID Gun_GetProjectileObject(const LARA_GUN_TYPE gun_type)
{
    const WEAPON_INFO *const info = Gun_Registry_Get(gun_type);
    return info->projectile_object_id;
}

LARA_GUN_TYPE Gun_GetTypeForProjectile(const OBJECT_ID obj_id)
{
    if (obj_id == NO_OBJECT) {
        return LGT_UNARMED;
    }
    for (int32_t i = 0; i < Gun_Registry_GetCount(); i++) {
        const WEAPON_INFO *const info = Gun_Registry_GetByIndex(i);
        if (info->projectile_object_id == obj_id) {
            return info->gun_type;
        }
    }
    return LGT_UNARMED;
}

bool Gun_IsLauncherType(const LARA_GUN_TYPE gun_type)
{
    const WEAPON_INFO *const info = Gun_Registry_Get(gun_type);
    return info->is_launcher;
}

bool Gun_IsRifleType(const LARA_GUN_TYPE gun_type)
{
    return M_IsGunType(gun_type, WEAPON_TYPE_RIFLE);
}

LARA_GUN_TYPE Gun_GetFlareType(void)
{
    for (int32_t i = 0; i < Gun_Registry_GetCount(); i++) {
        const LARA_GUN_TYPE gun_type = Gun_Registry_GetByIndex(i)->gun_type;
        if (Gun_IsFlareType(gun_type)) {
            return gun_type;
        }
    }
    return LGT_UNARMED;
}

bool Gun_IsFlareType(const LARA_GUN_TYPE gun_type)
{
    return M_IsGunType(gun_type, WEAPON_TYPE_FLARE);
}

bool Gun_IsDualPistolType(const LARA_GUN_TYPE gun_type)
{
    return M_IsGunType(gun_type, WEAPON_TYPE_DUAL_PISTOLS);
}

bool Gun_IsSinglePistolType(const LARA_GUN_TYPE gun_type)
{
    return M_IsGunType(gun_type, WEAPON_TYPE_SINGLE_PISTOL);
}

void Gun_SetLaraHandLMesh(const LARA_GUN_TYPE weapon_type)
{
    Lara_Skin_SetGunEquipment(LM_HAND_L, weapon_type);
}

void Gun_SetLaraHandRMesh(const LARA_GUN_TYPE weapon_type)
{
    Lara_Skin_SetGunEquipment(LM_HAND_R, weapon_type);
}

void Gun_SetLaraBackMesh(const LARA_GUN_TYPE weapon_type)
{
    Lara_Skin_SetGunEquipment(LM_TORSO, weapon_type);
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    lara_info->back_gun_type = weapon_type;
}

void Gun_SetLaraHolsterLMesh(const LARA_GUN_TYPE weapon_type)
{
    Lara_Skin_SetGunEquipment(LM_THIGH_L, weapon_type);
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    lara_info->holsters_gun_type = weapon_type;
}

void Gun_SetLaraHolsterRMesh(const LARA_GUN_TYPE weapon_type)
{
    Lara_Skin_SetGunEquipment(LM_THIGH_R, weapon_type);
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    lara_info->holsters_gun_type = weapon_type;
}
