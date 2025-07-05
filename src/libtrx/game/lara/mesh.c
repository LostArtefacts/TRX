#include "game/lara/mesh.h"

#include "game/gun.h"
#include "game/inventory.h"
#include "game/lara.h"
#include "game/savegame.h"

static LARA_GUN_TYPE M_DetermineHolsterGun(const RESUME_INFO *resume);
static LARA_GUN_TYPE M_DetermineBackGun(const RESUME_INFO *resume);

static LARA_GUN_TYPE M_DetermineHolsterGun(const RESUME_INFO *const resume)
{
    // TODO: merge this with the introduction of similar resume info setup and
    // manipulation during regular gameplay inventory modification.
#if TR_VERSION == 1
    const LARA_INFO *const lara_info = Lara_GetLaraInfo();
    if (lara_info->holsters_gun_type == LGT_UNARMED) {
        if (lara_info->gun_type != LGT_UNARMED
            && lara_info->gun_type != LGT_SHOTGUN) {
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
#else
    if (resume == nullptr) {
        return LGT_UNARMED;
    }

    switch (resume->equipped_gun_type) {
    case LGT_PISTOLS:
    case LGT_MAGNUMS:
    case LGT_UZIS:
        return resume->equipped_gun_type;
    default:
        if (resume->flags.has_magnums) {
            return LGT_MAGNUMS;
        } else if (resume->flags.has_uzis) {
            return LGT_UZIS;
        } else if (resume->flags.has_pistols) {
            return LGT_PISTOLS;
        }
        return LGT_UNARMED;
    }
#endif
}

static LARA_GUN_TYPE M_DetermineBackGun(const RESUME_INFO *const resume)
{
#if TR_VERSION == 1
    const LARA_INFO *const lara_info = Lara_GetLaraInfo();
    if (lara_info->back_gun_type == LGT_UNARMED
        && Inv_RequestItem(O_SHOTGUN_ITEM)) {
        return LGT_SHOTGUN;
    }
    return lara_info->back_gun_type;
#else
    if (resume == nullptr) {
        return LGT_UNARMED;
    }

    switch (resume->equipped_gun_type) {
    case LGT_M16:
    case LGT_GRENADE:
    case LGT_HARPOON:
        return resume->equipped_gun_type;
    default:
        break;
    }

    if (resume->flags.has_shotgun) {
        return LGT_SHOTGUN;
    } else if (resume->flags.has_m16) {
        return LGT_M16;
    } else if (resume->flags.has_grenade) {
        return LGT_GRENADE;
    } else if (resume->flags.has_harpoon) {
        return LGT_HARPOON;
    }
    return LGT_UNARMED;
#endif
}

void Lara_Mesh_Initialise(const GF_LEVEL *const level)
{
    const RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);

    const bool use_costume = resume != nullptr && resume->flags.costume;
    Lara_Mesh_SwapAll(use_costume ? O_LARA_EXTRA : O_LARA);
    Lara_Mesh_SwapSingle(LM_HEAD, O_LARA);

    const LARA_GUN_TYPE holster_gun = M_DetermineHolsterGun(resume);
    if (holster_gun != LGT_UNARMED) {
        Gun_SetLaraHolsterLMesh(holster_gun);
        Gun_SetLaraHolsterRMesh(holster_gun);
    }

    const LARA_GUN_TYPE back_gun = M_DetermineBackGun(resume);
    if (back_gun != LGT_UNARMED) {
        Gun_SetLaraBackMesh(back_gun);
    }

#if TR_VERSION >= 2
    if (resume->equipped_gun_type == LGT_FLARE) {
        Lara_Mesh_SwapSingle(LM_HAND_L, O_LARA_FLARE);
    }
#endif
}

void Lara_Mesh_SwapSingle(const LARA_MESH mesh, const GAME_OBJECT_ID obj_id)
{
    const OBJECT *const obj = Object_Get(obj_id);
    Lara_Mesh_Set(mesh, Object_GetMesh(obj->mesh_idx + mesh));
}

void Lara_Mesh_SwapAll(const GAME_OBJECT_ID obj_id)
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
