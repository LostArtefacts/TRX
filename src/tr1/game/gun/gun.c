#include "game/gun.h"

#include "game/camera.h"
#include "game/gun/gun_misc.h"
#include "game/gun/gun_pistols.h"
#include "game/gun/gun_rifle.h"
#include "game/input.h"
#include "game/inventory.h"
#include "game/lara/common.h"
#include "game/output.h"
#include "game/random.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/game/matrix.h>

void Gun_Control(void)
{
    if (g_Lara.left_arm.flash_gun > 0) {
        g_Lara.left_arm.flash_gun--;
    }
    if (g_Lara.right_arm.flash_gun > 0) {
        g_Lara.right_arm.flash_gun--;
    }

    bool draw = false;
    if (g_LaraItem->hit_points <= 0) {
        g_Lara.gun_status = LGS_ARMLESS;
    } else if (
        g_Lara.water_status == LWS_ABOVE_WATER
        || (g_Lara.water_status == LWS_WADE
            && g_Lara.water_surface_dist
                > -g_Weapons[g_Lara.gun_type].gun_height)) {
        if (g_Lara.request_gun_type != LGT_UNARMED
            && (g_Lara.request_gun_type != g_Lara.gun_type
                || g_Lara.gun_status == LGS_ARMLESS)) {
            if (g_Lara.gun_status == LGS_ARMLESS) {
                g_Lara.gun_type = g_Lara.request_gun_type;
                Gun_InitialiseNewWeapon();
                draw = true;
                g_Lara.request_gun_type = LGT_UNARMED;
            } else if (g_Lara.gun_status == LGS_READY) {
                draw = true;
            }
        } else if (g_Input.draw) {
            if (g_Lara.gun_type == LGT_UNARMED
                && Inv_RequestItem(O_PISTOL_ITEM)) {
                g_Lara.gun_type = LGT_PISTOLS;
                Gun_InitialiseNewWeapon();
            }
            draw = true;
            g_Lara.request_gun_type = LGT_UNARMED;
        }
    } else if (g_Lara.gun_status == LGS_READY) {
        draw = g_Lara.water_status != LWS_ABOVE_WATER
            && (g_Lara.water_status != LWS_WADE
                || g_Lara.water_surface_dist
                    < -g_Weapons[g_Lara.gun_type].gun_height);
    }

    if (draw && g_Lara.gun_type != LGT_UNARMED) {
        switch (g_Lara.gun_type) {
        case LGT_PISTOLS:
        case LGT_MAGNUMS:
        case LGT_UZIS:
            if (g_Lara.gun_status == LGS_ARMLESS) {
                g_Lara.gun_status = LGS_DRAW;
                g_Lara.right_arm.frame_num = LF_G_AIM_START;
                g_Lara.left_arm.frame_num = LF_G_AIM_START;
            } else if (g_Lara.gun_status == LGS_READY) {
                g_Lara.gun_status = LGS_UNDRAW;
            }
            break;

        case LGT_SHOTGUN:
            if (g_Lara.gun_status == LGS_ARMLESS) {
                g_Lara.gun_status = LGS_DRAW;
                g_Lara.left_arm.frame_num = LF_SG_AIM_START;
                g_Lara.right_arm.frame_num = LF_SG_AIM_START;
            } else if (g_Lara.gun_status == LGS_READY) {
                g_Lara.gun_status = LGS_UNDRAW;
            }
            break;

        default:
            break;
        }
    }

    switch (g_Lara.gun_status) {
    case LGS_DRAW:
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
            if (g_Camera.type != CAM_CINEMATIC && g_Camera.type != CAM_LOOK) {
                g_Camera.type = CAM_COMBAT;
            }
            Gun_Rifle_Draw(g_Lara.gun_type);
            break;

        default:
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
            Gun_Rifle_Undraw(g_Lara.gun_type);
            break;

        default:
            break;
        }
        break;

    case LGS_READY:
        Lara_SwapSingleMesh(LM_HEAD, O_LARA);

        switch (g_Lara.gun_type) {
        case LGT_PISTOLS:
            if (g_Lara.pistol_ammo.ammo && g_Input.action) {
                Lara_SwapSingleMesh(LM_HEAD, O_UZI_ANIM);
            }
            if (g_Camera.type != CAM_CINEMATIC && g_Camera.type != CAM_LOOK) {
                g_Camera.type = CAM_COMBAT;
            }
            Gun_Pistols_Control(g_Lara.gun_type);
            break;

        case LGT_MAGNUMS:
            if (g_Lara.magnum_ammo.ammo && g_Input.action) {
                Lara_SwapSingleMesh(LM_HEAD, O_UZI_ANIM);
            }
            if (g_Camera.type != CAM_CINEMATIC && g_Camera.type != CAM_LOOK) {
                g_Camera.type = CAM_COMBAT;
            }
            Gun_Pistols_Control(g_Lara.gun_type);
            break;

        case LGT_UZIS:
            if (g_Lara.uzi_ammo.ammo && g_Input.action) {
                Lara_SwapSingleMesh(LM_HEAD, O_UZI_ANIM);
            }
            if (g_Camera.type != CAM_CINEMATIC && g_Camera.type != CAM_LOOK) {
                g_Camera.type = CAM_COMBAT;
            }
            Gun_Pistols_Control(g_Lara.gun_type);
            break;

        case LGT_SHOTGUN:
            if (g_Lara.shotgun_ammo.ammo && g_Input.action) {
                Lara_SwapSingleMesh(LM_HEAD, O_UZI_ANIM);
            }
            if (g_Camera.type != CAM_CINEMATIC && g_Camera.type != CAM_LOOK) {
                g_Camera.type = CAM_COMBAT;
            }
            Gun_Rifle_Control(g_Lara.gun_type);
            break;

        default:
            break;
        }
        break;
    }
}

void Gun_InitialiseNewWeapon(void)
{
    g_Lara.left_arm.rot.x = 0;
    g_Lara.left_arm.rot.y = 0;
    g_Lara.left_arm.rot.z = 0;
    g_Lara.left_arm.lock = 0;
    g_Lara.left_arm.flash_gun = 0;
    g_Lara.left_arm.frame_num = LF_G_AIM_START;
    g_Lara.right_arm.rot.x = 0;
    g_Lara.right_arm.rot.y = 0;
    g_Lara.right_arm.rot.z = 0;
    g_Lara.right_arm.lock = 0;
    g_Lara.right_arm.flash_gun = 0;
    g_Lara.right_arm.frame_num = LF_G_AIM_START;
    g_Lara.target = nullptr;

    const GAME_OBJECT_ID anim_type = Gun_GetLaraAnim(g_Lara.gun_type);
    const OBJECT *const obj = Object_Get(anim_type);
    g_Lara.right_arm.frame_base = obj->frame_base;
    g_Lara.left_arm.frame_base = obj->frame_base;

    if (g_Lara.gun_status != LGS_ARMLESS) {
        if (anim_type == O_SHOTGUN_ANIM) {
            Gun_Rifle_DrawMeshes(g_Lara.gun_type);
        } else {
            Gun_Pistols_DrawMeshes(g_Lara.gun_type);
        }
    }
}

GAME_OBJECT_ID Gun_GetLaraAnim(const LARA_GUN_TYPE gun_type)
{
    switch (gun_type) {
    case LGT_PISTOLS:
    case LGT_MAGNUMS:
    case LGT_UZIS:
        return O_PISTOL_ANIM;
    case LGT_SHOTGUN:
        return O_SHOTGUN_ANIM;
    default:
        return O_LARA;
    }
}

GAME_OBJECT_ID Gun_GetWeaponAnim(const LARA_GUN_TYPE gun_type)
{
    switch (gun_type) {
    case LGT_PISTOLS:
        return O_PISTOL_ANIM;
    case LGT_MAGNUMS:
        return O_MAGNUM_ANIM;
    case LGT_UZIS:
        return O_UZI_ANIM;
    case LGT_SHOTGUN:
        return O_SHOTGUN_ANIM;
    default:
        return NO_OBJECT;
    }
}

LARA_GUN_TYPE Gun_GetType(const GAME_OBJECT_ID obj_id)
{
    switch (obj_id) {
    case O_PISTOL_ITEM:
        return LGT_PISTOLS;
    case O_MAGNUM_ITEM:
        return LGT_MAGNUMS;
    case O_UZI_ITEM:
        return LGT_UZIS;
    case O_SHOTGUN_ITEM:
        return LGT_SHOTGUN;
    default:
        return LGT_UNARMED;
    }
}

void Gun_DrawFlash(LARA_GUN_TYPE weapon_type, int32_t clip)
{
    int32_t light;
    int32_t len;
    int32_t off;

    switch (weapon_type) {
    case LGT_MAGNUMS:
        light = 16 * 256;
        len = 155;
        off = 55;
        break;

    case LGT_UZIS:
        light = 10 * 256;
        len = 180;
        off = 55;
        break;

    case LGT_SHOTGUN:
        if (!g_Config.visuals.enable_shotgun_flash) {
            return;
        }
        light = 10 * 256;
        len = 285;
        off = 0;
        break;

    default:
        light = 20 * 256;
        len = 155;
        off = 55;
        break;
    }

    Matrix_TranslateRel(0, len, off);
    Matrix_RotX(-90 * DEG_1);
    Matrix_RotZ((int16_t)(Random_GetDraw() * 2));
    Output_CalculateStaticLight(light);
    const OBJECT *const obj = Object_Get(O_GUN_FLASH);
    if (obj->loaded) {
        Object_DrawMesh(obj->mesh_idx, clip, false);
    }
}

void Gun_UpdateLaraMeshes(const GAME_OBJECT_ID obj_id)
{
    const bool lara_has_pistols = Inv_RequestItem(O_PISTOL_ITEM)
        || Inv_RequestItem(O_MAGNUM_ITEM) || Inv_RequestItem(O_UZI_ITEM);

    LARA_GUN_TYPE back_gun_type = LGT_UNARMED;
    LARA_GUN_TYPE holsters_gun_type = LGT_UNARMED;

    if (!Inv_RequestItem(O_SHOTGUN_ITEM) && obj_id == O_SHOTGUN_ITEM) {
        back_gun_type = LGT_SHOTGUN;
    } else if (!lara_has_pistols && obj_id == O_PISTOL_ITEM) {
        holsters_gun_type = LGT_PISTOLS;
    } else if (!lara_has_pistols && obj_id == O_MAGNUM_ITEM) {
        holsters_gun_type = LGT_MAGNUMS;
    } else if (!lara_has_pistols && obj_id == O_UZI_ITEM) {
        holsters_gun_type = LGT_UZIS;
    }

    if (back_gun_type != LGT_UNARMED) {
        Gun_SetLaraBackMesh(back_gun_type);
    }

    if (holsters_gun_type != LGT_UNARMED) {
        Gun_SetLaraHolsterLMesh(holsters_gun_type);
        Gun_SetLaraHolsterRMesh(holsters_gun_type);
    }
}

void Gun_SetLaraHandLMesh(const LARA_GUN_TYPE weapon_type)
{
    const GAME_OBJECT_ID obj_id =
        weapon_type == LGT_UNARMED ? O_LARA : Gun_GetWeaponAnim(weapon_type);
    ASSERT(obj_id != NO_OBJECT);
    Lara_SwapSingleMesh(LM_HAND_L, obj_id);
}

void Gun_SetLaraHandRMesh(const LARA_GUN_TYPE weapon_type)
{
    const GAME_OBJECT_ID obj_id =
        weapon_type == LGT_UNARMED ? O_LARA : Gun_GetWeaponAnim(weapon_type);
    ASSERT(obj_id != NO_OBJECT);
    Lara_SwapSingleMesh(LM_HAND_R, obj_id);
}

void Gun_SetLaraBackMesh(const LARA_GUN_TYPE weapon_type)
{
    const GAME_OBJECT_ID obj_id =
        weapon_type == LGT_UNARMED ? O_LARA : Gun_GetWeaponAnim(weapon_type);
    ASSERT(obj_id != NO_OBJECT);
    Lara_SwapSingleMesh(LM_TORSO, obj_id);
    g_Lara.back_gun_type = weapon_type;
}

void Gun_SetLaraHolsterLMesh(const LARA_GUN_TYPE weapon_type)
{
    const GAME_OBJECT_ID obj_id =
        weapon_type == LGT_UNARMED ? O_LARA : Gun_GetWeaponAnim(weapon_type);
    ASSERT(obj_id != NO_OBJECT);
    Lara_SwapSingleMesh(LM_THIGH_L, obj_id);
    g_Lara.holsters_gun_type = weapon_type;
}

void Gun_SetLaraHolsterRMesh(const LARA_GUN_TYPE weapon_type)
{
    const GAME_OBJECT_ID obj_id =
        weapon_type == LGT_UNARMED ? O_LARA : Gun_GetWeaponAnim(weapon_type);
    ASSERT(obj_id != NO_OBJECT);
    Lara_SwapSingleMesh(LM_THIGH_R, obj_id);
    g_Lara.holsters_gun_type = weapon_type;
}
