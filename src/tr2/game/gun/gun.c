#include "game/gun/gun.h"

#include "decomp/flares.h"
#include "game/camera.h"
#include "game/gun/gun_misc.h"
#include "game/gun/gun_pistols.h"
#include "game/gun/gun_rifle.h"
#include "game/input.h"
#include "game/inventory.h"
#include "game/lara/control.h"
#include "game/sound.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>

void Gun_Control(void)
{
    if (g_Lara.extra_anim) {
        g_Lara.request_gun_type = LGT_UNARMED;
        return;
    }

    if (g_Lara.left_arm.flash_gun > 0) {
        g_Lara.left_arm.flash_gun--;
    }

    if (g_Lara.right_arm.flash_gun > 0) {
        g_Lara.right_arm.flash_gun--;
    }

    if (g_LaraItem->hit_points <= 0) {
        g_Lara.gun_status = LGS_ARMLESS;
    } else if (g_Lara.gun_status == LGS_ARMLESS) {
        if (g_Input.draw) {
            g_Lara.request_gun_type = g_Lara.last_gun_type;
        } else if (g_InputDB.use_flare) {
            if (g_Lara.gun_type == LGT_FLARE) {
                g_Lara.gun_status = LGS_UNDRAW;
            } else if (
                Inv_RequestItem(O_FLARES_ITEM)
                && (!g_Config.gameplay.fix_free_flare_glitch
                    || g_LaraItem->current_anim_state != LS_PICKUP)) {
                g_Lara.request_gun_type = LGT_FLARE;
            }
        }

        if (g_Lara.request_gun_type != g_Lara.gun_type || g_Input.draw) {
            if (g_Lara.request_gun_type == LGT_FLARE
                || (g_Lara.skidoo == NO_ITEM && g_Lara.water_status != LWS_CHEAT
                    && (g_Lara.request_gun_type == LGT_HARPOON
                        || g_Lara.water_status == LWS_ABOVE_WATER
                        || (g_Lara.water_status == LWS_WADE
                            && g_Lara.water_surface_dist
                                > -g_Weapons[g_Lara.gun_type].gun_height)))) {
                if (g_Lara.gun_type == LGT_FLARE) {
                    Flare_Create(0);
                    Flare_UndrawMeshes();
                    g_Lara.flare_control_left = 0;
                }
                g_Lara.gun_type = g_Lara.request_gun_type;
                Gun_InitialiseNewWeapon();
                g_Lara.gun_status = LGS_DRAW;
                g_Lara.right_arm.frame_num = 0;
                g_Lara.left_arm.frame_num = 0;
            } else {
                g_Lara.last_gun_type = g_Lara.request_gun_type;
                if (g_Lara.gun_type == LGT_FLARE) {
                    g_Lara.request_gun_type = LGT_FLARE;
                } else {
                    g_Lara.gun_type = g_Lara.request_gun_type;
                }
            }
        }
    } else if (g_Lara.gun_status == LGS_READY) {
        if (g_InputDB.use_flare && Inv_RequestItem(O_FLARES_ITEM)) {
            g_Lara.request_gun_type = LGT_FLARE;
        }

        if (g_Input.draw || g_Lara.request_gun_type != g_Lara.gun_type) {
            g_Lara.gun_status = LGS_UNDRAW;
        } else if (
            g_Lara.gun_type == LGT_HARPOON
            && g_Lara.water_status == LWS_CHEAT) {
            g_Lara.gun_status = LGS_UNDRAW;
        } else if (
            g_Lara.gun_type != LGT_HARPOON
            && g_Lara.water_status != LWS_ABOVE_WATER
            && (g_Lara.water_status != LWS_WADE
                || g_Lara.water_surface_dist
                    < -g_Weapons[g_Lara.gun_type].gun_height)) {
            g_Lara.gun_status = LGS_UNDRAW;
        }
    }

    switch (g_Lara.gun_status) {
    case LGS_ARMLESS:
        if (g_Lara.gun_type == LGT_FLARE) {
            if (g_Lara.skidoo != NO_ITEM
                || Gun_CheckForHoldingState(g_LaraItem->current_anim_state)) {
                if (!g_Lara.flare_control_left) {
                    g_Lara.left_arm.frame_num = LF_FL_2_HOLD;
                    g_Lara.flare_control_left = 1;
                } else if (g_Lara.left_arm.frame_num != LF_FL_HOLD) {
                    g_Lara.left_arm.frame_num++;
                    if (g_Lara.left_arm.frame_num == LF_FL_END) {
                        g_Lara.left_arm.frame_num = LF_FL_HOLD;
                    }
                }
            } else {
                g_Lara.flare_control_left = 0;
            }
            Flare_DoInHand(g_Lara.flare_age);
            Flare_SetArm(g_Lara.left_arm.frame_num);
        }
        break;

    case LGS_HANDS_BUSY:
        if (g_Lara.gun_type == LGT_FLARE) {
            g_Lara.flare_control_left = g_Lara.skidoo != NO_ITEM
                || Gun_CheckForHoldingState(g_LaraItem->current_anim_state);
            Flare_DoInHand(g_Lara.flare_age);
            Flare_SetArm(g_Lara.left_arm.frame_num);
        }
        break;

    case LGS_DRAW:
        if (g_Lara.gun_type != LGT_FLARE && g_Lara.gun_type != LGT_UNARMED) {
            g_Lara.last_gun_type = g_Lara.gun_type;
        }

        switch (g_Lara.gun_type) {
        case LGT_PISTOLS:
        case LGT_MAGNUMS:
        case LGT_UZIS:
            if (g_Camera.type != CAM_CINEMATIC && g_Camera.type != CAM_LOOK) {
                g_Camera.type = CAM_COMBAT;
            }
            Gun_Pistols_Draw(g_Lara.gun_type);
            break;

        case LGT_SHOTGUN:
        case LGT_M16:
        case LGT_GRENADE:
        case LGT_HARPOON:
            if (g_Camera.type != CAM_CINEMATIC && g_Camera.type != CAM_LOOK) {
                g_Camera.type = CAM_COMBAT;
            }
            Gun_Rifle_Draw(g_Lara.gun_type);
            break;

        case LGT_FLARE:
            Flare_Draw();
            break;

        default:
            g_Lara.gun_status = LGS_ARMLESS;
            break;
        }
        break;

    case LGS_UNDRAW:
        Lara_SwapSingleMesh(LM_HEAD, O_LARA);

        switch (g_Lara.gun_type) {
        case LGT_PISTOLS:
        case LGT_MAGNUMS:
        case LGT_UZIS:
            Gun_Pistols_Undraw(g_Lara.gun_type);
            break;

        case LGT_SHOTGUN:
        case LGT_M16:
        case LGT_GRENADE:
        case LGT_HARPOON:
            Gun_Rifle_Undraw(g_Lara.gun_type);
            break;

        case LGT_FLARE:
            Flare_Undraw();
            break;
        default:
            return;
        }
        break;

    case LGS_READY:
        if (g_Lara.pistol_ammo.ammo && g_Input.action) {
            Lara_SwapSingleMesh(LM_HEAD, O_LARA_UZIS);
        } else {
            Lara_SwapSingleMesh(LM_HEAD, O_LARA);
        }

        if (g_Camera.type != CAM_CINEMATIC && g_Camera.type != CAM_LOOK) {
            g_Camera.type = CAM_COMBAT;
        }

        if (g_Input.action) {
            AMMO_INFO *const ammo = Gun_GetAmmoInfo(g_Lara.gun_type);
            ASSERT(ammo != nullptr);

            if (ammo->ammo <= 0) {
                ammo->ammo = 0;
                Sound_Effect(SFX_CLICK, &g_LaraItem->pos, SPM_NORMAL);
                g_Lara.request_gun_type =
                    Inv_RequestItem(O_PISTOL_ITEM) ? LGT_PISTOLS : LGT_UNARMED;
                break;
            }
        }

        switch (g_Lara.gun_type) {
        case LGT_PISTOLS:
        case LGT_MAGNUMS:
        case LGT_UZIS:
            Gun_Pistols_Control(g_Lara.gun_type);
            break;

        case LGT_SHOTGUN:
        case LGT_M16:
        case LGT_GRENADE:
        case LGT_HARPOON:
            Gun_Rifle_Control(g_Lara.gun_type);
            break;

        default:
            return;
        }
        break;

    case LGS_SPECIAL:
        Flare_Draw();
        break;

    default:
        return;
    }
}

void Gun_InitialiseNewWeapon(void)
{
    g_Lara.left_arm.flash_gun = 0;
    g_Lara.left_arm.frame_num = 0;
    g_Lara.left_arm.lock = 0;
    g_Lara.left_arm.rot.x = 0;
    g_Lara.left_arm.rot.y = 0;
    g_Lara.left_arm.rot.z = 0;
    g_Lara.right_arm.flash_gun = 0;
    g_Lara.right_arm.frame_num = 0;
    g_Lara.right_arm.lock = 0;
    g_Lara.right_arm.rot.x = 0;
    g_Lara.right_arm.rot.y = 0;
    g_Lara.right_arm.rot.z = 0;
    g_Lara.target = nullptr;

    switch (g_Lara.gun_type) {
    case LGT_PISTOLS:
    case LGT_MAGNUMS:
    case LGT_UZIS: {
        const OBJECT *const obj = Object_Get(O_LARA_PISTOLS);
        g_Lara.left_arm.frame_base = obj->frame_base;
        g_Lara.right_arm.frame_base = obj->frame_base;
        if (g_Lara.gun_status != LGS_ARMLESS) {
            Gun_Pistols_DrawMeshes(g_Lara.gun_type);
        }
        break;
    }

    case LGT_SHOTGUN:
    case LGT_M16:
    case LGT_GRENADE:
    case LGT_HARPOON: {
        const OBJECT *const obj =
            Object_Get(Gun_GetWeaponAnim(g_Lara.gun_type));
        g_Lara.left_arm.frame_base = obj->frame_base;
        g_Lara.right_arm.frame_base = obj->frame_base;
        if (g_Lara.gun_status != LGS_ARMLESS) {
            Gun_Rifle_DrawMeshes(g_Lara.gun_type);
        }
        break;
    }

    case LGT_FLARE: {
        const OBJECT *const obj = Object_Get(O_LARA_FLARE);
        g_Lara.left_arm.frame_base = obj->frame_base;
        g_Lara.right_arm.frame_base = obj->frame_base;
        if (g_Lara.gun_status != LGS_ARMLESS) {
            Flare_DrawMeshes();
        }
        break;
    }

    default:
        const ANIM *const anim = Item_GetAnim(g_LaraItem);
        g_Lara.left_arm.frame_base = anim->frame_ptr;
        g_Lara.right_arm.frame_base = anim->frame_ptr;
        break;
    }
}

int32_t Gun_GetWeaponAnim(const LARA_GUN_TYPE gun_type)
{
    switch (gun_type) {
    case LGT_UNARMED:
        return O_LARA;
    case LGT_PISTOLS:
        return O_LARA_PISTOLS;
    case LGT_MAGNUMS:
        return O_LARA_MAGNUMS;
    case LGT_UZIS:
        return O_LARA_UZIS;
    case LGT_SHOTGUN:
        return O_LARA_SHOTGUN;
    case LGT_M16:
        return O_LARA_M16;
    case LGT_GRENADE:
        return O_LARA_GRENADE;
    case LGT_HARPOON:
        return O_LARA_HARPOON;
    default:
        return NO_OBJECT;
    }
}

GAME_OBJECT_ID Gun_GetGunObject(const LARA_GUN_TYPE gun_type)
{
    // clang-format off
    switch (gun_type) {
    case LGT_PISTOLS: return O_PISTOL_ITEM;
    case LGT_MAGNUMS: return O_MAGNUM_ITEM;
    case LGT_UZIS:    return O_UZI_ITEM;
    case LGT_SHOTGUN: return O_SHOTGUN_ITEM;
    case LGT_HARPOON: return O_HARPOON_ITEM;
    case LGT_M16:     return O_M16_ITEM;
    case LGT_GRENADE: return O_GRENADE_ITEM;
    default:          return NO_OBJECT;
    }
    // clang-format on
}

GAME_OBJECT_ID Gun_GetAmmoObject(const LARA_GUN_TYPE gun_type)
{
    // clang-format off
    switch (gun_type) {
    case LGT_PISTOLS: return O_PISTOL_AMMO_ITEM;
    case LGT_MAGNUMS: return O_MAGNUM_AMMO_ITEM;
    case LGT_UZIS:    return O_UZI_AMMO_ITEM;
    case LGT_SHOTGUN: return O_SHOTGUN_AMMO_ITEM;
    case LGT_HARPOON: return O_HARPOON_AMMO_ITEM;
    case LGT_M16:     return O_M16_AMMO_ITEM;
    case LGT_GRENADE: return O_GRENADE_AMMO_ITEM;
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
    case LGT_HARPOON: return HARPOON_AMMO_QTY;
    case LGT_M16:     return M16_AMMO_QTY;
    case LGT_GRENADE: return GRENADE_AMMO_QTY;
    default:          return -1;
    }
    // clang-format on
}

AMMO_INFO *Gun_GetAmmoInfo(const LARA_GUN_TYPE gun_type)
{
    // clang-format off
    switch (gun_type) {
    case LGT_PISTOLS: return &g_Lara.pistol_ammo;
    case LGT_MAGNUMS: return &g_Lara.magnum_ammo;
    case LGT_UZIS:    return &g_Lara.uzi_ammo;
    case LGT_SHOTGUN: return &g_Lara.shotgun_ammo;
    case LGT_HARPOON: return &g_Lara.harpoon_ammo;
    case LGT_M16:     return &g_Lara.m16_ammo;
    case LGT_GRENADE: return &g_Lara.grenade_ammo;
    case LGT_SKIDOO:  return &g_Lara.pistol_ammo;
    default:          return nullptr;
    }
    // clang-format on
}

void Gun_SetLaraHandLMesh(const LARA_GUN_TYPE weapon_type)
{
    const GAME_OBJECT_ID obj_id = Gun_GetWeaponAnim(weapon_type);
    ASSERT(obj_id != NO_OBJECT);
    Lara_SwapSingleMesh(LM_HAND_L, obj_id);
}

void Gun_SetLaraHandRMesh(const LARA_GUN_TYPE weapon_type)
{
    const GAME_OBJECT_ID obj_id = Gun_GetWeaponAnim(weapon_type);
    ASSERT(obj_id != NO_OBJECT);
    Lara_SwapSingleMesh(LM_HAND_R, obj_id);
}

void Gun_SetLaraBackMesh(const LARA_GUN_TYPE weapon_type)
{
    const GAME_OBJECT_ID obj_id = Gun_GetWeaponAnim(weapon_type);
    ASSERT(obj_id != NO_OBJECT);
    Lara_SwapSingleMesh(LM_TORSO, obj_id);
    g_Lara.back_gun = obj_id;
}

void Gun_SetLaraHolsterLMesh(const LARA_GUN_TYPE weapon_type)
{
    const GAME_OBJECT_ID obj_id = Gun_GetWeaponAnim(weapon_type);
    ASSERT(obj_id != NO_OBJECT);
    Lara_SwapSingleMesh(LM_THIGH_L, obj_id);
}

void Gun_SetLaraHolsterRMesh(const LARA_GUN_TYPE weapon_type)
{
    const GAME_OBJECT_ID obj_id = Gun_GetWeaponAnim(weapon_type);
    ASSERT(obj_id != NO_OBJECT);
    Lara_SwapSingleMesh(LM_THIGH_R, obj_id);
}
