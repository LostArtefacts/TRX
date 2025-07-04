#include "game/lara/control.h"

#include "game/gun/gun.h"
#include "game/inventory.h"
#include "game/savegame.h"
#include "game/sound.h"
#include "game/stats.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/lara.h>

void Lara_UseItem(const GAME_OBJECT_ID obj_id)
{
    ITEM *const item = g_LaraItem;

    switch (obj_id) {
    case O_PISTOL_ITEM:
    case O_PISTOL_OPTION:
        g_Lara.request_gun_type = LGT_PISTOLS;
        break;

    case O_SHOTGUN_ITEM:
    case O_SHOTGUN_OPTION:
        g_Lara.request_gun_type = LGT_SHOTGUN;
        break;

    case O_MAGNUM_ITEM:
    case O_MAGNUM_OPTION:
        g_Lara.request_gun_type = LGT_MAGNUMS;
        break;

    case O_UZI_ITEM:
    case O_UZI_OPTION:
        g_Lara.request_gun_type = LGT_UZIS;
        break;

    case O_HARPOON_ITEM:
    case O_HARPOON_OPTION:
        g_Lara.request_gun_type = LGT_HARPOON;
        break;

    case O_M16_ITEM:
    case O_M16_OPTION:
        g_Lara.request_gun_type = LGT_M16;
        break;

    case O_GRENADE_ITEM:
    case O_GRENADE_OPTION:
        g_Lara.request_gun_type = LGT_GRENADE;
        break;

    case O_SMALL_MEDIPACK_ITEM:
    case O_SMALL_MEDIPACK_OPTION:
        if (item->hit_points > 0 && item->hit_points < LARA_MAX_HITPOINTS) {
            item->hit_points += LARA_MAX_HITPOINTS / 2;
            CLAMPG(item->hit_points, LARA_MAX_HITPOINTS);
            Inv_RemoveItem(O_SMALL_MEDIPACK_ITEM);
            Sound_Effect(SFX_MENU_MEDI, nullptr, SPM_ALWAYS);
            Stats_AddMedipacksUsed(0.5);
        }
        break;

    case O_LARGE_MEDIPACK_ITEM:
    case O_LARGE_MEDIPACK_OPTION:
        if (item->hit_points > 0 && item->hit_points < LARA_MAX_HITPOINTS) {
            item->hit_points = LARA_MAX_HITPOINTS;
            Inv_RemoveItem(O_LARGE_MEDIPACK_ITEM);
            Sound_Effect(SFX_MENU_MEDI, nullptr, SPM_ALWAYS);
            Stats_AddMedipacksUsed(1);
        }
        break;

    case O_FLARES_ITEM:
    case O_FLARES_OPTION:
        g_Lara.request_gun_type = LGT_FLARE;
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
    case O_PUZZLE_OPTION_4: {
        const int16_t receptacle_item_num = Object_FindReceptacle(obj_id);
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

void Lara_InitialiseLoad(const int16_t item_num)
{
    g_Lara.item_num = item_num;
    g_LaraItem = Item_Get(item_num);
}

void Lara_InitialiseInventory(const GF_LEVEL *const level)
{
    Inv_RemoveAllItems();
    Inv_AddItem(O_COMPASS_OPTION);

    RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);
    if (resume != nullptr) {
        g_Lara.pistol_ammo.ammo = 1000;
        if (resume->flags.has_pistols) {
            Inv_AddItem(O_PISTOL_ITEM);
        }

        if (resume->flags.has_magnums) {
            Inv_AddItem(O_MAGNUM_ITEM);
            g_Lara.magnum_ammo.ammo = resume->magnum_ammo;
            Item_GlobalReplace(O_MAGNUM_ITEM, O_MAGNUM_AMMO_ITEM);
        } else {
            Inv_AddItemNTimes(O_MAGNUM_AMMO_ITEM, resume->magnum_ammo / 40);
            g_Lara.magnum_ammo.ammo = 0;
        }

        if (resume->flags.has_uzis) {
            Inv_AddItem(O_UZI_ITEM);
            g_Lara.uzi_ammo.ammo = resume->uzi_ammo;
            Item_GlobalReplace(O_UZI_ITEM, O_UZI_AMMO_ITEM);
        } else {
            Inv_AddItemNTimes(O_UZI_AMMO_ITEM, resume->uzi_ammo / 80);
            g_Lara.uzi_ammo.ammo = 0;
        }

        if (resume->flags.has_shotgun) {
            Inv_AddItem(O_SHOTGUN_ITEM);
            g_Lara.shotgun_ammo.ammo = resume->shotgun_ammo;
            Item_GlobalReplace(O_SHOTGUN_ITEM, O_SHOTGUN_AMMO_ITEM);
        } else {
            Inv_AddItemNTimes(O_SHOTGUN_AMMO_ITEM, resume->shotgun_ammo / 12);
            g_Lara.shotgun_ammo.ammo = 0;
        }

        if (resume->flags.has_m16) {
            Inv_AddItem(O_M16_ITEM);
            g_Lara.m16_ammo.ammo = resume->m16_ammo;
            Item_GlobalReplace(O_M16_ITEM, O_M16_AMMO_ITEM);
        } else {
            Inv_AddItemNTimes(O_M16_AMMO_ITEM, resume->m16_ammo / 40);
            g_Lara.m16_ammo.ammo = 0;
        }

        if (resume->flags.has_grenade) {
            Inv_AddItem(O_GRENADE_ITEM);
            g_Lara.grenade_ammo.ammo = resume->grenade_ammo;
            Item_GlobalReplace(O_GRENADE_ITEM, O_GRENADE_AMMO_ITEM);
        } else {
            Inv_AddItemNTimes(O_GRENADE_AMMO_ITEM, resume->grenade_ammo / 2);
            g_Lara.grenade_ammo.ammo = 0;
        }

        if (resume->flags.has_harpoon) {
            Inv_AddItem(O_HARPOON_ITEM);
            g_Lara.harpoon_ammo.ammo = resume->harpoon_ammo;
            Item_GlobalReplace(O_HARPOON_ITEM, O_HARPOON_AMMO_ITEM);
        } else {
            Inv_AddItemNTimes(O_HARPOON_AMMO_ITEM, resume->harpoon_ammo / 3);
            g_Lara.harpoon_ammo.ammo = 0;
        }

        Inv_AddItemNTimes(O_FLARE_ITEM, resume->flares);
        Inv_AddItemNTimes(O_SMALL_MEDIPACK_ITEM, resume->small_medipacks);
        Inv_AddItemNTimes(O_LARGE_MEDIPACK_ITEM, resume->large_medipacks);

        g_Lara.last_gun_type = resume->equipped_gun_type;
    }

    g_Lara.gun_status = LGS_ARMLESS;
    g_Lara.gun_type = g_Lara.last_gun_type;
    g_Lara.request_gun_type = g_Lara.last_gun_type;

    Lara_InitialiseMeshes(level);
    Gun_InitialiseNewWeapon();
}

void Lara_InitialiseMeshes(const GF_LEVEL *const level)
{
    for (int32_t i = 0; i < LM_NUMBER_OF; i++) {
        Lara_SwapSingleMesh(i, O_LARA);
    }

    const RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);
    if (resume == nullptr) {
        return;
    }

    GAME_OBJECT_ID holster_obj_id = NO_OBJECT;
    if (resume->equipped_gun_type != LGT_UNARMED) {
        if (resume->equipped_gun_type == LGT_MAGNUMS) {
            holster_obj_id = O_LARA_MAGNUMS;
        } else if (resume->equipped_gun_type == LGT_UZIS) {
            holster_obj_id = O_LARA_UZIS;
        } else {
            holster_obj_id = O_LARA_PISTOLS;
        }
    }

    if (holster_obj_id != NO_OBJECT) {
        Lara_SwapSingleMesh(LM_THIGH_L, holster_obj_id);
        Lara_SwapSingleMesh(LM_THIGH_R, holster_obj_id);
    }

    if (resume->equipped_gun_type == LGT_FLARE) {
        Lara_SwapSingleMesh(LM_HAND_L, O_LARA_FLARE);
    }

    switch (resume->equipped_gun_type) {
    case LGT_M16:
        g_Lara.back_gun_obj_id = O_LARA_M16;
        return;

    case LGT_GRENADE:
        g_Lara.back_gun_obj_id = O_LARA_GRENADE;
        return;

    case LGT_HARPOON:
        g_Lara.back_gun_obj_id = O_LARA_HARPOON;
        return;

    default:
        break;
    }

    if (resume->flags.has_shotgun) {
        g_Lara.back_gun_obj_id = O_LARA_SHOTGUN;
    } else if (resume->flags.has_m16) {
        g_Lara.back_gun_obj_id = O_LARA_M16;
    } else if (resume->flags.has_grenade) {
        g_Lara.back_gun_obj_id = O_LARA_GRENADE;
    } else if (resume->flags.has_harpoon) {
        g_Lara.back_gun_obj_id = O_LARA_HARPOON;
    }
}
