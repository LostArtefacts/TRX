#include "game/gun.h"

#include "game/gun/gun_pistols.h"
#include "game/gun/gun_rifle.h"
#include "game/input.h"
#include "game/inventory.h"
#include "game/output.h"
#include "game/random.h"
#include "global/const.h"
#include "global/vars.h"
#include "math/matrix.h"

#include <stdbool.h>
#include <stddef.h>

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
    } else if (g_Lara.water_status == LWS_ABOVE_WATER) {
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
            if (g_Lara.gun_type == LGT_UNARMED && Inv_RequestItem(O_GUN_ITEM)) {
                g_Lara.gun_type = LGT_PISTOLS;
                Gun_InitialiseNewWeapon();
            }
            draw = true;
            g_Lara.request_gun_type = LGT_UNARMED;
        }
    } else if (g_Lara.gun_status == LGS_READY) {
        draw = true;
    }

    if (draw && g_Lara.gun_type != LGT_UNARMED) {
        switch (g_Lara.gun_type) {
        case LGT_PISTOLS:
        case LGT_MAGNUMS:
        case LGT_UZIS:
            if (g_Lara.gun_status == LGS_ARMLESS) {
                g_Lara.gun_status = LGS_DRAW;
                g_Lara.right_arm.frame_number = LF_G_AIM_START;
                g_Lara.left_arm.frame_number = LF_G_AIM_START;
            } else if (g_Lara.gun_status == LGS_READY) {
                g_Lara.gun_status = LGS_UNDRAW;
            }
            break;

        case LGT_SHOTGUN:
            if (g_Lara.gun_status == LGS_ARMLESS) {
                g_Lara.gun_status = LGS_DRAW;
                g_Lara.left_arm.frame_number = LF_SG_AIM_START;
                g_Lara.right_arm.frame_number = LF_SG_AIM_START;
            } else if (g_Lara.gun_status == LGS_READY) {
                g_Lara.gun_status = LGS_UNDRAW;
            }
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
            Gun_Rifle_Draw();
            break;
        }
        break;

    case LGS_UNDRAW:
        g_Lara.mesh_ptrs[LM_HEAD] =
            g_Meshes[g_Objects[O_LARA].mesh_index + LM_HEAD];
        switch (g_Lara.gun_type) {
        case LGT_PISTOLS:
        case LGT_MAGNUMS:
        case LGT_UZIS:
            Gun_Pistols_Undraw(g_Lara.gun_type);
            break;

        case LGT_SHOTGUN:
            Gun_Rifle_Undraw();
            break;
        }
        break;

    case LGS_READY:
        g_Lara.mesh_ptrs[LM_HEAD] =
            g_Meshes[g_Objects[O_LARA].mesh_index + LM_HEAD];
        switch (g_Lara.gun_type) {
        case LGT_PISTOLS:
            if (g_Lara.pistols.ammo && g_Input.action) {
                g_Lara.mesh_ptrs[LM_HEAD] =
                    g_Meshes[g_Objects[O_UZI].mesh_index + LM_HEAD];
            }
            if (g_Camera.type != CAM_CINEMATIC && g_Camera.type != CAM_LOOK) {
                g_Camera.type = CAM_COMBAT;
            }
            Gun_Pistols_Control(g_Lara.gun_type);
            break;

        case LGT_MAGNUMS:
            if (g_Lara.magnums.ammo && g_Input.action) {
                g_Lara.mesh_ptrs[LM_HEAD] =
                    g_Meshes[g_Objects[O_UZI].mesh_index + LM_HEAD];
            }
            if (g_Camera.type != CAM_CINEMATIC && g_Camera.type != CAM_LOOK) {
                g_Camera.type = CAM_COMBAT;
            }
            Gun_Pistols_Control(g_Lara.gun_type);
            break;

        case LGT_UZIS:
            if (g_Lara.uzis.ammo && g_Input.action) {
                g_Lara.mesh_ptrs[LM_HEAD] =
                    g_Meshes[g_Objects[O_UZI].mesh_index + LM_HEAD];
            }
            if (g_Camera.type != CAM_CINEMATIC && g_Camera.type != CAM_LOOK) {
                g_Camera.type = CAM_COMBAT;
            }
            Gun_Pistols_Control(g_Lara.gun_type);
            break;

        case LGT_SHOTGUN:
            if (g_Lara.shotgun.ammo && g_Input.action) {
                g_Lara.mesh_ptrs[LM_HEAD] =
                    g_Meshes[g_Objects[O_UZI].mesh_index + LM_HEAD];
            }
            if (g_Camera.type != CAM_CINEMATIC && g_Camera.type != CAM_LOOK) {
                g_Camera.type = CAM_COMBAT;
            }
            Gun_Rifle_Control(LGT_SHOTGUN);
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
    g_Lara.left_arm.frame_number = LF_G_AIM_START;
    g_Lara.right_arm.rot.x = 0;
    g_Lara.right_arm.rot.y = 0;
    g_Lara.right_arm.rot.z = 0;
    g_Lara.right_arm.lock = 0;
    g_Lara.right_arm.flash_gun = 0;
    g_Lara.right_arm.frame_number = LF_G_AIM_START;
    g_Lara.target = NULL;

    const GAME_OBJECT_ID anim_type = Gun_GetLaraAnimation(g_Lara.gun_type);
    g_Lara.right_arm.frame_base = g_Objects[anim_type].frame_base;
    g_Lara.left_arm.frame_base = g_Objects[anim_type].frame_base;

    if (g_Lara.gun_status != LGS_ARMLESS) {
        if (anim_type == O_SHOTGUN) {
            Gun_Rifle_DrawMeshes();
        } else {
            Gun_Pistols_DrawMeshes(g_Lara.gun_type);
        }
    }
}

GAME_OBJECT_ID Gun_GetLaraAnimation(LARA_GUN_TYPE gun_type)
{
    switch (gun_type) {
    case LGT_PISTOLS:
    case LGT_MAGNUMS:
    case LGT_UZIS:
        return O_PISTOLS;
    case LGT_SHOTGUN:
        return O_SHOTGUN;
    default:
        return O_LARA;
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
    Matrix_RotYXZ(0, -90 * PHD_DEGREE, (PHD_ANGLE)(Random_GetDraw() * 2));
    Output_CalculateStaticLight(light);
    if (g_Objects[O_GUN_FLASH].loaded) {
        Output_DrawPolygons(g_Meshes[g_Objects[O_GUN_FLASH].mesh_index], clip);
    }
}
