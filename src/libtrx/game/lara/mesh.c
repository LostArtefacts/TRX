#include "game/lara/mesh.h"

#include "game/gun.h"
#include "game/lara.h"
#include "game/savegame.h"

static LARA_GUN_TYPE M_DetermineHolsterGun(const RESUME_INFO *resume);
static LARA_GUN_TYPE M_DetermineBackGun(const RESUME_INFO *resume);

static LARA_GUN_TYPE M_DetermineHolsterGun(const RESUME_INFO *const resume)
{
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
}

static LARA_GUN_TYPE M_DetermineBackGun(const RESUME_INFO *const resume)
{
    if (resume == nullptr) {
        return LGT_UNARMED;
    }

#if TR_VERSION == 1
    if (resume->flags.has_shotgun) {
        return LGT_SHOTGUN;
    }
#else
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
#endif

    return LGT_UNARMED;
}

void Lara_Mesh_Initialise(const GF_LEVEL *const level)
{
    const RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);

    const bool use_costume = resume != nullptr && resume->flags.costume
        && Object_Get(O_LARA_EXTRA)->loaded;
    for (LARA_MESH mesh = LM_FIRST; mesh < LM_NUMBER_OF; mesh++) {
        Lara_SwapSingleMesh(
            mesh, mesh == LM_HEAD || !use_costume ? O_LARA : O_LARA_EXTRA);
    }

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
        Lara_SwapSingleMesh(LM_HAND_L, O_LARA_FLARE);
    }
#endif
}
