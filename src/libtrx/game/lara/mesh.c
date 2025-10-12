#include "game/lara/mesh.h"

#include "game/gun.h"
#include "game/inventory.h"
#include "game/lara.h"
#include "game/savegame.h"

static LARA_GUN_TYPE M_DetermineHolsterGun(void)
{
    const LARA_INFO *const lara_info = Lara_GetLaraInfo();
    if (lara_info->holsters_gun_type == LGT_UNARMED) {
        if (lara_info->gun_type != LGT_UNARMED
            && !Gun_IsRifleType(lara_info->gun_type)) {
            return lara_info->gun_type;
        } else if (Inv_RequestItem(O_PISTOL_ITEM)) {
            return LGT_PISTOLS;
        } else if (Inv_RequestItem(O_MAGNUM_ITEM)) {
            return LGT_MAGNUMS;
        } else if (Inv_RequestItem(O_UZI_ITEM)) {
            return LGT_UZIS;
        }
    }
    return lara_info->holsters_gun_type;
}

static LARA_GUN_TYPE M_DetermineBackGun(void)
{
    const LARA_INFO *const lara_info = Lara_GetLaraInfo();
    if (lara_info->back_gun_type != LGT_UNARMED) {
        return lara_info->back_gun_type;
    }

    if (Inv_RequestItem(O_SHOTGUN_ITEM)) {
        return LGT_SHOTGUN;
    }
#if TR_VERSION >= 2
    else if (Inv_RequestItem(O_M16_ITEM)) {
        return LGT_M16;
    } else if (Inv_RequestItem(O_GRENADE_ITEM)) {
        return LGT_GRENADE;
    } else if (Inv_RequestItem(O_HARPOON_ITEM)) {
        return LGT_HARPOON;
    }
#endif
    return LGT_UNARMED;
}

void Lara_Mesh_Initialise(const GF_LEVEL *const level)
{
    const RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);

    const bool use_costume = resume != nullptr && resume->flags.costume;
    Lara_Mesh_SwapAll(use_costume ? O_LARA_EXTRA : O_LARA);
    Lara_Mesh_SwapSingle(LM_HEAD, O_LARA);

    const LARA_GUN_TYPE holster_gun = M_DetermineHolsterGun();
    if (holster_gun != LGT_UNARMED) {
        Gun_SetLaraHolsterLMesh(holster_gun);
        Gun_SetLaraHolsterRMesh(holster_gun);
    }

    const LARA_GUN_TYPE back_gun = M_DetermineBackGun();
    if (back_gun != LGT_UNARMED) {
        Gun_SetLaraBackMesh(back_gun);
    }

#if TR_VERSION >= 2
    if (resume->equipped_gun_type == LGT_FLARE) {
        Lara_Mesh_SwapSingle(LM_HAND_L, O_LARA_FLARE);
    }
#endif
}

void Lara_Mesh_SwapSingle(const LARA_MESH mesh, const OBJECT_ID obj_id)
{
    const OBJECT *const obj = Object_Get(obj_id);
    Lara_Mesh_Set(mesh, Object_GetMesh(obj->mesh_idx + mesh));
}

void Lara_Mesh_SwapAll(const OBJECT_ID obj_id)
{
    if (!Object_Get(obj_id)->loaded) {
        return;
    }

    for (LARA_MESH mesh = LM_FIRST; mesh < LM_NUMBER_OF; mesh++) {
        Lara_Mesh_SwapSingle(mesh, obj_id);
    }
}

void Lara_Mesh_Set(const LARA_MESH mesh, OBJECT_MESH *const mesh_ptr)
{
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    lara_info->mesh_ptrs[mesh] = mesh_ptr;
}

OBJECT_MESH *Lara_Mesh_Get(const LARA_MESH mesh)
{
    const LARA_INFO *const lara_info = Lara_GetLaraInfo();
    return lara_info->mesh_ptrs[mesh];
}
