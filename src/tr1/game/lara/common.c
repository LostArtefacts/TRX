#include "game/lara/common.h"

#include "game/game.h"
#include "game/game_flow.h"
#include "game/gun.h"
#include "game/input.h"
#include "game/inventory.h"
#include "game/item_actions.h"
#include "game/objects/common.h"
#include "game/objects/vars.h"
#include "game/savegame.h"
#include "game/sound.h"
#include "game/spawn.h"
#include "game/stats.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/math.h>
#include <libtrx/log.h>
#include <libtrx/utils.h>

LARA_INFO *Lara_GetLaraInfo(void)
{
    return &g_Lara;
}

ITEM *Lara_GetItem(void)
{
    return g_LaraItem;
}

void Lara_SwapMeshExtra(void)
{
    if (!Object_Get(O_LARA_EXTRA)->loaded) {
        return;
    }
    for (LARA_MESH mesh = LM_FIRST; mesh < LM_NUMBER_OF; mesh++) {
        Lara_SwapSingleMesh(mesh, O_LARA_EXTRA);
    }
}

void Lara_UseItem(const GAME_OBJECT_ID obj_id)
{
    LOG_INFO("%d", obj_id);
    switch (obj_id) {
    case O_PISTOL_ITEM:
    case O_PISTOL_OPTION:
        g_Lara.request_gun_type = LGT_PISTOLS;
        if (g_Lara.gun_status == LGS_ARMLESS
            && g_Lara.gun_type == LGT_PISTOLS) {
            g_Lara.gun_type = LGT_UNARMED;
        }
        break;

    case O_SHOTGUN_ITEM:
    case O_SHOTGUN_OPTION:
        g_Lara.request_gun_type = LGT_SHOTGUN;
        if (g_Lara.gun_status == LGS_ARMLESS
            && g_Lara.gun_type == LGT_SHOTGUN) {
            g_Lara.gun_type = LGT_UNARMED;
        }
        break;

    case O_MAGNUM_ITEM:
    case O_MAGNUM_OPTION:
        g_Lara.request_gun_type = LGT_MAGNUMS;
        if (g_Lara.gun_status == LGS_ARMLESS
            && g_Lara.gun_type == LGT_MAGNUMS) {
            g_Lara.gun_type = LGT_UNARMED;
        }
        break;

    case O_UZI_ITEM:
    case O_UZI_OPTION:
        g_Lara.request_gun_type = LGT_UZIS;
        if (g_Lara.gun_status == LGS_ARMLESS && g_Lara.gun_type == LGT_UZIS) {
            g_Lara.gun_type = LGT_UNARMED;
        }
        break;

    case O_SMALL_MEDIPACK_ITEM:
    case O_SMALL_MEDIPACK_OPTION:
        if (g_LaraItem->hit_points <= 0
            || g_LaraItem->hit_points >= LARA_MAX_HITPOINTS) {
            return;
        }
        g_LaraItem->hit_points += LARA_MAX_HITPOINTS / 2;
        CLAMPG(g_LaraItem->hit_points, LARA_MAX_HITPOINTS);
        Inv_RemoveItem(O_SMALL_MEDIPACK_ITEM);
        Sound_Effect(SFX_MENU_MEDI, nullptr, SPM_ALWAYS);
        Stats_AddMedipacksUsed(.5);
        break;

    case O_LARGE_MEDIPACK_ITEM:
    case O_LARGE_MEDIPACK_OPTION:
        if (g_LaraItem->hit_points <= 0
            || g_LaraItem->hit_points >= LARA_MAX_HITPOINTS) {
            return;
        }
        g_LaraItem->hit_points = g_LaraItem->hit_points + LARA_MAX_HITPOINTS;
        CLAMPG(g_LaraItem->hit_points, LARA_MAX_HITPOINTS);
        Inv_RemoveItem(O_LARGE_MEDIPACK_ITEM);
        Sound_Effect(SFX_MENU_MEDI, nullptr, SPM_ALWAYS);
        Stats_AddMedipacksUsed(1);
        break;

    case O_KEY_ITEM_1:
    case O_KEY_OPTION_1:
    case O_KEY_ITEM_2:
    case O_KEY_OPTION_2:
    case O_KEY_ITEM_3:
    case O_KEY_OPTION_3:
    case O_KEY_ITEM_4:
    case O_KEY_OPTION_4:
    case O_PUZZLE_ITEM_1:
    case O_PUZZLE_OPTION_1:
    case O_PUZZLE_ITEM_2:
    case O_PUZZLE_OPTION_2:
    case O_PUZZLE_ITEM_3:
    case O_PUZZLE_OPTION_3:
    case O_PUZZLE_ITEM_4:
    case O_PUZZLE_OPTION_4:
    case O_LEADBAR_ITEM:
    case O_LEADBAR_OPTION:
    case O_SCION_ITEM_1:
    case O_SCION_ITEM_2:
    case O_SCION_ITEM_3:
    case O_SCION_ITEM_4:
    case O_SCION_OPTION: {
        int16_t receptacle_item_num = Object_FindReceptacle(obj_id);
        if (receptacle_item_num == NO_ITEM) {
            Sound_Effect(SFX_LARA_NO, nullptr, SPM_NORMAL);
            return;
        }
        if (g_Lara.interact_target.item_num != NO_ITEM) {
            Sound_Effect(SFX_LARA_NO, nullptr, SPM_NORMAL);
            return;
        }
        g_Lara.interact_target.item_num = receptacle_item_num;
        g_Lara.interact_target.is_moving = true;
        g_Lara.interact_target.move_count = 0;
        break;
    }

    default:
        break;
    }
}

void Lara_InitialiseLoad(int16_t item_num)
{
    g_Lara.item_num = item_num;
    if (item_num == NO_ITEM) {
        g_LaraItem = nullptr;
    } else {
        g_LaraItem = Item_Get(item_num);
    }
}

void Lara_InitialiseInventory(const GF_LEVEL *const level)
{
    Inv_RemoveAllItems();

    RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);

    g_Lara.pistol_ammo.ammo = 1000;

    if (resume != nullptr) {
        if (g_GameInfo.remove_guns) {
            resume->flags.has_pistols = false;
            resume->flags.has_shotgun = false;
            resume->flags.has_magnums = false;
            resume->flags.has_uzis = false;
            resume->equipped_gun_type = LGT_UNARMED;
            resume->holsters_gun_type = LGT_UNARMED;
            resume->back_gun_type = LGT_UNARMED;
            resume->gun_status = LGS_ARMLESS;
        }

        if (g_GameInfo.remove_scions) {
            resume->num_scions = 0;
        }

        if (g_GameInfo.remove_ammo) {
            resume->shotgun_ammo = 0;
            resume->magnum_ammo = 0;
            resume->uzi_ammo = 0;
        }

        if (g_GameInfo.remove_medipacks) {
            resume->small_medipacks = 0;
            resume->large_medipacks = 0;
        }

        if (resume->flags.has_pistols) {
            Inv_AddItem(O_PISTOL_ITEM);
        }

        if (resume->flags.has_magnums) {
            Inv_AddItem(O_MAGNUM_ITEM);
            g_Lara.magnum_ammo.ammo = resume->magnum_ammo;
            Item_GlobalReplace(O_MAGNUM_ITEM, O_MAGNUM_AMMO_ITEM);
        } else {
            int32_t ammo = resume->magnum_ammo / MAGNUM_AMMO_QTY;
            for (int i = 0; i < ammo; i++) {
                Inv_AddItem(O_MAGNUM_AMMO_ITEM);
            }
            g_Lara.magnum_ammo.ammo = 0;
        }

        if (resume->flags.has_uzis) {
            Inv_AddItem(O_UZI_ITEM);
            g_Lara.uzi_ammo.ammo = resume->uzi_ammo;
            Item_GlobalReplace(O_UZI_ITEM, O_UZI_AMMO_ITEM);
        } else {
            int32_t ammo = resume->uzi_ammo / UZI_AMMO_QTY;
            for (int i = 0; i < ammo; i++) {
                Inv_AddItem(O_UZI_AMMO_ITEM);
            }
            g_Lara.uzi_ammo.ammo = 0;
        }

        if (resume->flags.has_shotgun) {
            Inv_AddItem(O_SHOTGUN_ITEM);
            g_Lara.shotgun_ammo.ammo = resume->shotgun_ammo;
            Item_GlobalReplace(O_SHOTGUN_ITEM, O_SHOTGUN_AMMO_ITEM);
        } else {
            int32_t ammo = resume->shotgun_ammo / SHOTGUN_AMMO_QTY;
            for (int i = 0; i < ammo; i++) {
                Inv_AddItem(O_SHOTGUN_AMMO_ITEM);
            }
            g_Lara.shotgun_ammo.ammo = 0;
        }

        for (int i = 0; i < resume->num_scions; i++) {
            Inv_AddItem(O_SCION_ITEM_1);
        }

        for (int i = 0; i < resume->small_medipacks; i++) {
            Inv_AddItem(O_SMALL_MEDIPACK_ITEM);
        }

        for (int i = 0; i < resume->large_medipacks; i++) {
            Inv_AddItem(O_LARGE_MEDIPACK_ITEM);
        }

        g_Lara.gun_status = resume->gun_status;
        g_Lara.gun_type = resume->equipped_gun_type;
        g_Lara.request_gun_type = resume->equipped_gun_type;
        g_Lara.holsters_gun_type = resume->holsters_gun_type;
        g_Lara.back_gun_type = resume->back_gun_type;
    }

    Lara_InitialiseMeshes(level);
    Gun_InitialiseNewWeapon();
}

void Lara_RevertToPistolsIfNeeded(void)
{
    if (!g_Config.gameplay.revert_to_pistols
        || !Inv_RequestItem(O_PISTOL_ITEM)) {
        return;
    }

    g_Lara.gun_type = LGT_PISTOLS;

    if (g_Lara.gun_status != LGS_ARMLESS) {
        g_Lara.holsters_gun_type = LGT_UNARMED;
    }
    if (Inv_RequestItem(O_SHOTGUN_ITEM)) {
        g_Lara.back_gun_type = LGT_SHOTGUN;
    } else {
        g_Lara.back_gun_type = LGT_UNARMED;
    }
    Gun_InitialiseNewWeapon();
    Gun_SetLaraHolsterLMesh(g_Lara.holsters_gun_type);
    Gun_SetLaraHolsterRMesh(g_Lara.holsters_gun_type);
    Gun_SetLaraBackMesh(g_Lara.back_gun_type);
}

void Lara_InitialiseMeshes(const GF_LEVEL *const level)
{
    const RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);
    const bool use_costume = resume != nullptr && resume->flags.costume
        && Object_Get(O_LARA_EXTRA)->loaded;

    for (LARA_MESH mesh = LM_FIRST; mesh < LM_NUMBER_OF; mesh++) {
        Lara_SwapSingleMesh(
            mesh, mesh == LM_HEAD || !use_costume ? O_LARA : O_LARA_EXTRA);
    }

    LARA_GUN_TYPE back_gun_type = g_Lara.back_gun_type;
    LARA_GUN_TYPE holsters_gun_type = g_Lara.holsters_gun_type;

    if (back_gun_type == LGT_UNARMED && Inv_RequestItem(O_SHOTGUN_ITEM)) {
        back_gun_type = LGT_SHOTGUN;
    }

    if (holsters_gun_type == LGT_UNARMED) {
        if (g_Lara.gun_type != LGT_UNARMED && g_Lara.gun_type != LGT_SHOTGUN) {
            holsters_gun_type = g_Lara.gun_type;
        } else if (Inv_RequestItem(O_PISTOL_ITEM)) {
            holsters_gun_type = LGT_PISTOLS;
        } else if (Inv_RequestItem(O_MAGNUM_ITEM)) {
            holsters_gun_type = LGT_MAGNUMS;
        } else if (Inv_RequestItem(O_UZI_ITEM)) {
            holsters_gun_type = LGT_UZIS;
        }
    }

    if (back_gun_type != LGT_UNARMED && back_gun_type != LGT_UNKNOWN) {
        Gun_SetLaraBackMesh(back_gun_type);
    }

    if (holsters_gun_type != LGT_UNARMED && holsters_gun_type != LGT_UNKNOWN) {
        Gun_SetLaraHolsterLMesh(holsters_gun_type);
        Gun_SetLaraHolsterRMesh(holsters_gun_type);
    }
}
