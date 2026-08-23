#include <trx/config.h>
#include <trx/core/colors.h>
#include <trx/debug.h>
#include <trx/game/const.h>
#include <trx/game/game.h>
#include <trx/game/gun.h>
#include <trx/game/lara.h>
#include <trx/game/output.h>
#include <trx/version.h>

#define M_SHOTGUN_ROUNDS_PER_SHOT 6

// What a weapon is made of outside the numbers weapons.json5 carries: the
// pickup lying in the world, the box of ammunition for it, and the animation
// object Lara holds it with. A weapon a game has no object for reads
// NO_OBJECT, as do the flare and the skidoo, which are held like weapons
// without being ones.
// clang-format off
static const struct {
    OBJECT_ID gun;
    OBJECT_ID ammo;
    OBJECT_ID anim;
} m_WeaponObjects[NUM_WEAPONS] = {
    [LGT_UNARMED]      = { NO_OBJECT,            NO_OBJECT,                NO_OBJECT           },
    [LGT_PISTOLS]      = { O_PISTOL_ITEM,        O_PISTOL_AMMO_ITEM,       O_LARA_PISTOLS      },
    [LGT_MAGNUMS]      = { O_MAGNUM_ITEM,        O_MAGNUM_AMMO_ITEM,       O_LARA_MAGNUMS      },
    [LGT_UZIS]         = { O_UZI_ITEM,           O_UZI_AMMO_ITEM,          O_LARA_UZIS         },
    [LGT_SHOTGUN]      = { O_SHOTGUN_ITEM,       O_SHOTGUN_AMMO_ITEM,      O_LARA_SHOTGUN      },
    [LGT_M16]          = { O_M16_ITEM,           O_M16_AMMO_ITEM,          O_LARA_M16          },
    [LGT_GRENADE]      = { O_GRENADE_GUN_ITEM,   O_GRENADE_AMMO_ITEM,      O_LARA_GRENADE_GUN  },
    [LGT_HARPOON]      = { O_HARPOON_ITEM,       O_HARPOON_AMMO_ITEM,      O_LARA_HARPOON_GUN  },
    [LGT_FLARE]        = { NO_OBJECT,            NO_OBJECT,                O_LARA_FLARE        },
    [LGT_SKIDOO]       = { NO_OBJECT,            NO_OBJECT,                NO_OBJECT           },
    [LGT_AUTOS]        = { O_AUTOS_ITEM,         O_AUTOS_AMMO_ITEM,        O_LARA_AUTOS        },
    [LGT_DESERT_EAGLE] = { O_DESERT_EAGLE_ITEM,  O_DESERT_EAGLE_AMMO_ITEM, O_LARA_DESERT_EAGLE },
    [LGT_MP5]          = { O_MP5_ITEM,           O_MP5_AMMO_ITEM,          O_LARA_MP5          },
    [LGT_ROCKET]       = { O_ROCKET_GUN_ITEM,    O_ROCKET_AMMO_ITEM,       O_LARA_ROCKET_GUN   },
    [LGT_CROSSBOW]     = { O_CROSSBOW_ITEM,      O_CROSSBOW_AMMO_1_ITEM,   O_LARA_CROSSBOW     },
    [LGT_REVOLVER]     = { O_REVOLVER_ITEM,      O_REVOLVER_AMMO_ITEM,     O_LARA_REVOLVER     },
};
// clang-format on

// Which weapon Lara wears where, when she carries more than one that could
// hang there. The weapon type says which of the two slots a weapon can use -
// the rifles go on her back and the rest into her holsters - but not which of
// them wins, so the order is here.
// clang-format off
static const LARA_GUN_TYPE m_HolsterGuns[] = {
    LGT_PISTOLS, LGT_MAGNUMS, LGT_AUTOS, LGT_DESERT_EAGLE, LGT_UZIS,
    LGT_REVOLVER, LGT_UNARMED,
};

static const LARA_GUN_TYPE m_BackGuns[] = {
    LGT_SHOTGUN, LGT_M16, LGT_MP5, LGT_GRENADE, LGT_ROCKET, LGT_HARPOON,
    LGT_CROSSBOW, LGT_UNARMED,
};
// clang-format on

static LARA_GUN_TYPE M_GetFirstCarried(
    const INVENTORY_STATE *const inv, const LARA_GUN_TYPE *const gun_types)
{
    for (int32_t i = 0; gun_types[i] != LGT_UNARMED; i++) {
        if (Inv_State_Has(inv, Gun_GetGunObject(gun_types[i]))) {
            return gun_types[i];
        }
    }
    return LGT_UNARMED;
}

// Whether the type addresses a row of the table above. A legacy save can hold
// LGT_UNKNOWN, which is below the first row.
static bool M_IsWeapon(const LARA_GUN_TYPE gun_type)
{
    return gun_type >= LGT_UNARMED && gun_type < NUM_WEAPONS;
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
    return M_IsGunType(gun_type, WEAPON_TYPE_DUAL_PISTOLS)
        ? O_LARA_PISTOLS
        : Gun_GetWeaponAnim(gun_type);
}

OBJECT_ID Gun_GetWeaponAnim(const LARA_GUN_TYPE gun_type)
{
    // A legacy save can name a gun type the engine no longer knows, and
    // unarmed has no weapon of its own; both hold Lara's plain animations.
    if (gun_type <= LGT_UNARMED) {
        return O_LARA;
    }
    return M_IsWeapon(gun_type) ? m_WeaponObjects[gun_type].anim : NO_OBJECT;
}

LARA_GUN_TYPE Gun_GetType(const OBJECT_ID obj_id)
{
    if (obj_id != NO_OBJECT) {
        for (LARA_GUN_TYPE gun_type = LGT_UNARMED; gun_type < NUM_WEAPONS;
             gun_type++) {
            if (m_WeaponObjects[gun_type].gun == obj_id) {
                return gun_type;
            }
        }
    }
    return LGT_UNARMED;
}

OBJECT_ID Gun_GetGunObject(const LARA_GUN_TYPE gun_type)
{
    return M_IsWeapon(gun_type) ? m_WeaponObjects[gun_type].gun : NO_OBJECT;
}

OBJECT_ID Gun_GetAmmoObject(const LARA_GUN_TYPE gun_type)
{
    return M_IsWeapon(gun_type) ? m_WeaponObjects[gun_type].ammo : NO_OBJECT;
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
    return gun_type == LGT_SHOTGUN ? M_SHOTGUN_ROUNDS_PER_SHOT : 1;
}

LARA_GUN_TYPE Gun_GetHolsterChoice(const INVENTORY_STATE *const inv)
{
    return M_GetFirstCarried(inv, m_HolsterGuns);
}

LARA_GUN_TYPE Gun_GetBackChoice(const INVENTORY_STATE *const inv)
{
    return M_GetFirstCarried(inv, m_BackGuns);
}

bool Gun_HasInfiniteAmmo(const LARA_GUN_TYPE gun_type)
{
    if (Game_IsBonusFlagSet(GBF_NGPLUS)) {
        return true;
    }
    return M_IsWeapon(gun_type) && Gun_Registry_Get(gun_type)->ammo.infinite;
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

bool Gun_IsRifleType(const LARA_GUN_TYPE gun_type)
{
    return M_IsGunType(gun_type, WEAPON_TYPE_RIFLE);
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
