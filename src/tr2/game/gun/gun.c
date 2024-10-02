#include "game/gun/gun.h"

#include "game/gun/gun_misc.h"
#include "game/gun/gun_pistols.h"
#include "game/gun/gun_rifle.h"
#include "game/input.h"
#include "game/inventory/backpack.h"
#include "game/lara/control.h"
#include "game/sound.h"
#include "global/funcs.h"
#include "global/vars.h"

#include <assert.h>

typedef enum {
    LF_FL_HOLD_FT = 1,
    LF_FL_THROW_FT = 32,
    LF_FL_DRAW_FT = 39,
    LF_FL_IGNITE_FT = 23,
    LF_FL_2_HOLD_FT = 15,

    LF_FL_HOLD = 0,
    LF_FL_THROW = (LF_FL_HOLD + LF_FL_HOLD_FT), // = 1
    LF_FL_DRAW = (LF_FL_THROW + LF_FL_THROW_FT), // = 33
    LF_FL_IGNITE = (LF_FL_DRAW + LF_FL_DRAW_FT), // = 72
    LF_FL_2_HOLD = (LF_FL_IGNITE + LF_FL_IGNITE_FT), // = 95
    LF_FL_END = (LF_FL_2_HOLD + LF_FL_2_HOLD_FT), // = 110
} LARA_FLARE_ANIMATION_FRAME;

void __cdecl Gun_Control(void)
{
    if (g_Lara.left_arm.flash_gun > 0) {
        g_Lara.left_arm.flash_gun--;
    }

    if (g_Lara.right_arm.flash_gun > 0) {
        g_Lara.right_arm.flash_gun--;
    }

    if (g_LaraItem->hit_points <= 0) {
        g_Lara.gun_status = LGS_ARMLESS;
    } else if (g_Lara.gun_status == LGS_ARMLESS) {
        if (g_Input & IN_DRAW) {
            g_Lara.request_gun_type = g_Lara.last_gun_type;
        } else if (g_InputDB & IN_FLARE) {
            if (g_Lara.gun_type == LGT_FLARE) {
                g_Lara.gun_status = LGS_UNDRAW;
            } else if (Inv_RequestItem(O_FLARES_ITEM)) {
                g_Lara.request_gun_type = LGT_FLARE;
            }
        }

        if (g_Lara.request_gun_type != g_Lara.gun_type || (g_Input & IN_DRAW)) {
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
        if ((g_InputDB & IN_FLARE) && Inv_RequestItem(O_FLARES_ITEM)) {
            g_Lara.request_gun_type = LGT_FLARE;
        }

        if ((g_Input & IN_DRAW) || g_Lara.request_gun_type != g_Lara.gun_type) {
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
        g_Lara.mesh_ptrs[LM_HEAD] =
            g_Meshes[g_Objects[O_LARA].mesh_idx + LM_HEAD];

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
        if (g_Lara.pistol_ammo.ammo && (g_Input & IN_ACTION)) {
            g_Lara.mesh_ptrs[LM_HEAD] =
                g_Meshes[g_Objects[O_LARA_UZIS].mesh_idx + LM_HEAD];
        } else {
            g_Lara.mesh_ptrs[LM_HEAD] =
                g_Meshes[g_Objects[O_LARA].mesh_idx + LM_HEAD];
        }

        if (g_Camera.type != CAM_CINEMATIC && g_Camera.type != CAM_LOOK) {
            g_Camera.type = CAM_COMBAT;
        }

        if (g_Input & IN_ACTION) {
            AMMO_INFO *const ammo = Gun_GetAmmoInfo(g_Lara.gun_type);
            assert(ammo != NULL);

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

void __cdecl Gun_InitialiseNewWeapon(void)
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
    g_Lara.target = NULL;

    switch (g_Lara.gun_type) {
    case LGT_PISTOLS:
    case LGT_MAGNUMS:
    case LGT_UZIS:
        g_Lara.left_arm.frame_base = g_Objects[O_LARA_PISTOLS].frame_base;
        g_Lara.right_arm.frame_base = g_Objects[O_LARA_PISTOLS].frame_base;
        if (g_Lara.gun_status != LGS_ARMLESS) {
            Gun_Pistols_DrawMeshes(g_Lara.gun_type);
        }
        break;

    case LGT_SHOTGUN:
    case LGT_M16:
    case LGT_GRENADE:
    case LGT_HARPOON:
        g_Lara.left_arm.frame_base =
            g_Objects[Gun_GetWeaponAnim(g_Lara.gun_type)].frame_base;
        g_Lara.right_arm.frame_base =
            g_Objects[Gun_GetWeaponAnim(g_Lara.gun_type)].frame_base;
        if (g_Lara.gun_status != LGS_ARMLESS) {
            Gun_Rifle_DrawMeshes(g_Lara.gun_type);
        }
        break;

    case LGT_FLARE:
        g_Lara.left_arm.frame_base = g_Objects[O_LARA_FLARE].frame_base;
        g_Lara.right_arm.frame_base = g_Objects[O_LARA_FLARE].frame_base;
        if (g_Lara.gun_status != LGS_ARMLESS) {
            Flare_DrawMeshes();
        }
        break;

    default:
        g_Lara.left_arm.frame_base = g_Anims[g_LaraItem->anim_num].frame_ptr;
        g_Lara.right_arm.frame_base = g_Anims[g_LaraItem->anim_num].frame_ptr;
        break;
    }
}

int32_t __cdecl Gun_GetWeaponAnim(const LARA_GUN_TYPE gun_type)
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
    default:          return NULL;
    }
    // clang-format on
}

void Gun_SetLaraHandLMesh(const LARA_GUN_TYPE weapon_type)
{
    const GAME_OBJECT_ID object_id = Gun_GetWeaponAnim(weapon_type);
    assert(object_id != NO_OBJECT);
    Lara_SwapSingleMesh(LM_HAND_L, object_id);
}

void Gun_SetLaraHandRMesh(const LARA_GUN_TYPE weapon_type)
{
    const GAME_OBJECT_ID object_id = Gun_GetWeaponAnim(weapon_type);
    assert(object_id != NO_OBJECT);
    Lara_SwapSingleMesh(LM_HAND_R, object_id);
}

void Gun_SetLaraBackMesh(const LARA_GUN_TYPE weapon_type)
{
    const GAME_OBJECT_ID object_id = Gun_GetWeaponAnim(weapon_type);
    assert(object_id != NO_OBJECT);
    Lara_SwapSingleMesh(LM_TORSO, object_id);
    g_Lara.back_gun = object_id;
}

void Gun_SetLaraHolsterLMesh(const LARA_GUN_TYPE weapon_type)
{
    const GAME_OBJECT_ID object_id = Gun_GetWeaponAnim(weapon_type);
    assert(object_id != NO_OBJECT);
    Lara_SwapSingleMesh(LM_THIGH_L, object_id);
}

void Gun_SetLaraHolsterRMesh(const LARA_GUN_TYPE weapon_type)
{
    const GAME_OBJECT_ID object_id = Gun_GetWeaponAnim(weapon_type);
    assert(object_id != NO_OBJECT);
    Lara_SwapSingleMesh(LM_THIGH_R, object_id);
}
