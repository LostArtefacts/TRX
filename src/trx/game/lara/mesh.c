#include <trx/game/lara/mesh.h>

#include <trx/config.h>
#include <trx/game/gun.h>
#include <trx/game/inventory.h>
#include <trx/game/lara.h>
#include <trx/game/savegame.h>

typedef enum {
    BRAID_TORSO,
    BRAID_HEAD_NORMAL,
    BRAID_HEAD_UZI,
    BRAID_HEAD_TREX,
} M_BRAID_BODY_MESH;

static bool m_BraidStatus = false;

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
        } else if (Inv_RequestItem(O_AUTOS_ITEM)) {
            return LGT_AUTOS;
        } else if (Inv_RequestItem(O_DESERT_EAGLE_ITEM)) {
            return LGT_DESERT_EAGLE;
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
    } else if (Inv_RequestItem(O_M16_ITEM)) {
        return LGT_M16;
    } else if (Inv_RequestItem(O_MP5_ITEM)) {
        return LGT_MP5;
    } else if (Inv_RequestItem(O_GRENADE_GUN_ITEM)) {
        return LGT_GRENADE;
    } else if (Inv_RequestItem(O_ROCKET_GUN_ITEM)) {
        return LGT_ROCKET;
    } else if (Inv_RequestItem(O_HARPOON_ITEM)) {
        return LGT_HARPOON;
    }
    return LGT_UNARMED;
}

static void M_SwapHairParts(void)
{
    Object_SwapMeshEx(O_LARA, O_LARA_HAIR_BODY_SWAP, LM_TORSO, BRAID_TORSO);
    Object_SwapMeshEx(
        O_LARA, O_LARA_HAIR_BODY_SWAP, LM_HEAD, BRAID_HEAD_NORMAL);
    Object_SwapMeshEx(
        O_LARA_UZIS, O_LARA_HAIR_BODY_SWAP, LM_HEAD, BRAID_HEAD_UZI);
    Object_SwapMeshEx(
        O_LARA_EXTRA_SKIN_TREX, O_LARA_HAIR_BODY_SWAP, LM_HEAD,
        BRAID_HEAD_TREX);
}

static OBJECT_ID M_GetTargetMesh(const LARA_MESH mesh_idx)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    const ITEM *const item = Lara_GetItem();

    if (lara->extra_anim) {
        switch (item->current_anim_state) {
        case LS_EXTRA_TREX_KILL:
            return O_LARA_EXTRA_SKIN_TREX;
        case LS_EXTRA_MIDAS_KILL:
            if ((lara->mesh_effects & (1 << mesh_idx)) != 0) {
                return O_LARA_EXTRA_SKIN_MIDAS;
            }
        default:
            break;
        }
        return O_LARA;
    }

    const GF_LEVEL *const level = GF_GetCurrentLevel();
    const RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);
    if (resume != nullptr && resume->flags.costume && mesh_idx != LM_HEAD) {
        // For the context of replacing Lara's meshes when the braid has been
        // toggled, all meshes in the gym outfit apart from her head must remain
        // as the default.
        return O_LARA_EXTRA;
    }

    return O_LARA;
}

static void M_InitialiseCutsceneLevel(void)
{
    Lara_Mesh_SwapAll(O_LARA);
    Lara_Mesh_SwapSingle(LM_THIGH_L, O_LARA_PISTOLS);
    Lara_Mesh_SwapSingle(LM_THIGH_R, O_LARA_PISTOLS);
}

static void M_InitialiseNormalLevel(const GF_LEVEL *const level)
{
    const RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);

    const bool use_costume = resume != nullptr && resume->flags.costume;
    Lara_Mesh_SwapAll(use_costume ? O_LARA_EXTRA : O_LARA);
    Lara_Mesh_SwapSingle(LM_HEAD, O_LARA);

    const LARA_GUN_TYPE holster_gun = M_DetermineHolsterGun();
    if (holster_gun != LGT_UNARMED && holster_gun != LGT_FLARE) {
        Gun_SetLaraHolsterLMesh(holster_gun);
        Gun_SetLaraHolsterRMesh(holster_gun);
    }

    const LARA_GUN_TYPE back_gun = M_DetermineBackGun();
    if (back_gun != LGT_UNARMED) {
        Gun_SetLaraBackMesh(back_gun);
    }

    if (resume != nullptr && resume->equipped_gun_type == LGT_FLARE) {
        Lara_Mesh_SwapSingle(LM_HAND_L, O_LARA_FLARE);
    }
}

void Lara_Mesh_Initialise(const GF_LEVEL *const level)
{
    const OBJECT *const skin_obj = Object_Get(O_LARA_SKIN);
    if (skin_obj->loaded) {
        OBJECT *const lara_obj = Object_Get(O_LARA);
        lara_obj->mesh_idx = skin_obj->mesh_idx;
    }

    if (level->type == GFL_CUTSCENE) {
        M_InitialiseCutsceneLevel();
    } else {
        M_InitialiseNormalLevel(level);
    }

    m_BraidStatus = false;
    Lara_Mesh_UpdateHair(false);
}

void Lara_Mesh_UpdateHair(const bool enforce)
{
    if (m_BraidStatus != g_Config.visuals.enable_braid
        || (m_BraidStatus && enforce)) {
        M_SwapHairParts();
        m_BraidStatus = g_Config.visuals.enable_braid;
    }

    Lara_Mesh_SwapSingle(LM_TORSO, M_GetTargetMesh(LM_TORSO));
    Lara_Mesh_SwapSingle(LM_HEAD, M_GetTargetMesh(LM_HEAD));
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
