#include "config.h"
#include "debug.h"
#include "game/const.h"
#include "game/gun.h"
#include "game/lara.h"
#include "game/output.h"

void Gun_AddDynamicLight(void)
{
    if (!g_Config.visuals.enable_gun_lighting) {
        return;
    }

    const ITEM *const lara_item = Lara_GetItem();
    const int32_t c = Math_Cos(lara_item->rot.y);
    const int32_t s = Math_Sin(lara_item->rot.y);
    const XYZ_32 pos = {
        .x = lara_item->pos.x + (s >> (W2V_SHIFT - 10)),
        .y = lara_item->pos.y - WALL_L / 2,
        .z = lara_item->pos.z + (c >> (W2V_SHIFT - 10)),
    };
    Output_AddDynamicLight(pos, 12, 11);
}

OBJECT_ID Gun_GetLaraAnim(const LARA_GUN_TYPE gun_type)
{
    if (TR_VERSION == 1) {
        switch (gun_type) {
        case LGT_PISTOLS:
        case LGT_MAGNUMS:
        case LGT_UZIS:
            return O_LARA_PISTOLS;
        default:
            break;
        }
    }
    return Gun_GetWeaponAnim(gun_type);
}

OBJECT_ID Gun_GetWeaponAnim(const LARA_GUN_TYPE gun_type)
{
    // clang-format off
    switch (gun_type) {
    case LGT_UNKNOWN: return O_LARA;
    case LGT_UNARMED: return O_LARA;
    case LGT_PISTOLS: return O_LARA_PISTOLS;
    case LGT_MAGNUMS: return O_LARA_MAGNUMS;
    case LGT_UZIS:    return O_LARA_UZIS;
    case LGT_SHOTGUN: return O_LARA_SHOTGUN;
#if TR_VERSION >= 2
    case LGT_M16:     return O_LARA_M16;
    case LGT_GRENADE: return O_LARA_GRENADE;
    case LGT_HARPOON: return O_LARA_HARPOON;
    case LGT_FLARE:   return O_LARA_FLARE;
#endif
    default:          return NO_OBJECT;
    }
    // clang-format on
}

LARA_GUN_TYPE Gun_GetType(const OBJECT_ID obj_id)
{
    // clang-format off
    switch (obj_id) {
    case O_PISTOL_ITEM:  return LGT_PISTOLS;
    case O_MAGNUM_ITEM:  return LGT_MAGNUMS;
    case O_UZI_ITEM:     return LGT_UZIS;
    case O_SHOTGUN_ITEM: return LGT_SHOTGUN;
#if TR_VERSION >= 2
    case O_HARPOON_ITEM: return LGT_HARPOON;
    case O_M16_ITEM:     return LGT_M16;
    case O_GRENADE_ITEM: return LGT_GRENADE;
#endif
    default:             return LGT_UNARMED;
    }
    // clang-format on
}

OBJECT_ID Gun_GetGunObject(const LARA_GUN_TYPE gun_type)
{
    // clang-format off
    switch (gun_type) {
    case LGT_PISTOLS: return O_PISTOL_ITEM;
    case LGT_MAGNUMS: return O_MAGNUM_ITEM;
    case LGT_UZIS:    return O_UZI_ITEM;
    case LGT_SHOTGUN: return O_SHOTGUN_ITEM;
#if TR_VERSION >= 2
    case LGT_HARPOON: return O_HARPOON_ITEM;
    case LGT_M16:     return O_M16_ITEM;
    case LGT_GRENADE: return O_GRENADE_ITEM;
#endif
    default:          return NO_OBJECT;
    }
    // clang-format on
}

OBJECT_ID Gun_GetAmmoObject(const LARA_GUN_TYPE gun_type)
{
    // clang-format off
    switch (gun_type) {
    case LGT_PISTOLS: return O_PISTOL_AMMO_ITEM;
    case LGT_MAGNUMS: return O_MAGNUM_AMMO_ITEM;
    case LGT_UZIS:    return O_UZI_AMMO_ITEM;
    case LGT_SHOTGUN: return O_SHOTGUN_AMMO_ITEM;
#if TR_VERSION >= 2
    case LGT_HARPOON: return O_HARPOON_AMMO_ITEM;
    case LGT_M16:     return O_M16_AMMO_ITEM;
    case LGT_GRENADE: return O_GRENADE_AMMO_ITEM;
#endif
    default:          return NO_OBJECT;
    }
    // clang-format on
}

int32_t Gun_GetAmmoQuantity(const LARA_GUN_TYPE gun_type)
{
    // clang-format off
    switch (gun_type) {
    case LGT_PISTOLS: return 1;
    case LGT_MAGNUMS: return MAGNUM_AMMO_QTY;
    case LGT_UZIS:    return UZI_AMMO_QTY;
    case LGT_SHOTGUN: return SHOTGUN_AMMO_QTY;
#if TR_VERSION >= 2
    case LGT_HARPOON: return HARPOON_AMMO_QTY;
    case LGT_M16:     return M16_AMMO_QTY;
    case LGT_GRENADE: return GRENADE_AMMO_QTY;
#endif
    default:          return -1;
    }
    // clang-format on
}

AMMO_INFO *Gun_GetAmmoInfo(const LARA_GUN_TYPE gun_type)
{
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    if (lara_info == nullptr) {
        return nullptr;
    }
    // clang-format off
    switch (gun_type) {
    case LGT_PISTOLS: return &lara_info->pistol_ammo;
    case LGT_MAGNUMS: return &lara_info->magnum_ammo;
    case LGT_UZIS:    return &lara_info->uzi_ammo;
    case LGT_SHOTGUN: return &lara_info->shotgun_ammo;
#if TR_VERSION >= 2
    case LGT_HARPOON: return &lara_info->harpoon_ammo;
    case LGT_M16:     return &lara_info->m16_ammo;
    case LGT_GRENADE: return &lara_info->grenade_ammo;
    case LGT_SKIDOO:  return &lara_info->pistol_ammo;
#endif
    default:          return nullptr;
    }
    // clang-format on
}

bool Gun_IsRifleType(const LARA_GUN_TYPE gun_type)
{
    switch (gun_type) {
    case LGT_SHOTGUN:
#if TR_VERSION >= 2
    case LGT_M16:
    case LGT_GRENADE:
    case LGT_HARPOON:
#endif
        return true;
    default:
        return false;
    }
}

void Gun_SetLaraHandLMesh(const LARA_GUN_TYPE weapon_type)
{
    const OBJECT_ID obj_id = Gun_GetWeaponAnim(weapon_type);
    ASSERT(obj_id != NO_OBJECT);
    Lara_Mesh_SwapSingle(LM_HAND_L, obj_id);
}

void Gun_SetLaraHandRMesh(const LARA_GUN_TYPE weapon_type)
{
    const OBJECT_ID obj_id = Gun_GetWeaponAnim(weapon_type);
    ASSERT(obj_id != NO_OBJECT);
    Lara_Mesh_SwapSingle(LM_HAND_R, obj_id);
}

void Gun_SetLaraBackMesh(const LARA_GUN_TYPE weapon_type)
{
    const OBJECT_ID obj_id =
        weapon_type == LGT_UNARMED ? O_LARA : Gun_GetWeaponAnim(weapon_type);
    ASSERT(obj_id != NO_OBJECT);
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
#if TR_VERSION == 1
    Lara_Mesh_SwapSingle(LM_TORSO, obj_id);
    lara_info->back_gun_type = weapon_type;
#else
    lara_info->back_gun_obj_id = obj_id;
    lara_info->back_gun_type =
        weapon_type == LGT_UNARMED ? lara_info->last_gun_type : weapon_type;
#endif
}

void Gun_SetLaraHolsterLMesh(const LARA_GUN_TYPE weapon_type)
{
    const OBJECT_ID obj_id = Gun_GetWeaponAnim(weapon_type);
    ASSERT(obj_id != NO_OBJECT);
    Lara_Mesh_SwapSingle(LM_THIGH_L, obj_id);
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    lara_info->holsters_gun_type = weapon_type;
}

void Gun_SetLaraHolsterRMesh(const LARA_GUN_TYPE weapon_type)
{
    const OBJECT_ID obj_id = Gun_GetWeaponAnim(weapon_type);
    ASSERT(obj_id != NO_OBJECT);
    Lara_Mesh_SwapSingle(LM_THIGH_R, obj_id);
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    lara_info->holsters_gun_type = weapon_type;
}
