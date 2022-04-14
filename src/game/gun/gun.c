#include "game/gun/gun.h"

#include "game/gun/gun_pistols.h"
#include "game/gun/gun_rifle.h"
#include "game/input.h"
#include "game/inv.h"
#include "global/vars.h"

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
                g_Lara.right_arm.frame_number = AF_G_AIM;
                g_Lara.left_arm.frame_number = AF_G_AIM;
            } else if (g_Lara.gun_status == LGS_READY) {
                g_Lara.gun_status = LGS_UNDRAW;
            }
            break;

        case LGT_SHOTGUN:
            if (g_Lara.gun_status == LGS_ARMLESS) {
                g_Lara.gun_status = LGS_DRAW;
                g_Lara.left_arm.frame_number = AF_SG_AIM;
                g_Lara.right_arm.frame_number = AF_SG_AIM;
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
    g_Lara.left_arm.x_rot = 0;
    g_Lara.left_arm.y_rot = 0;
    g_Lara.left_arm.z_rot = 0;
    g_Lara.left_arm.lock = 0;
    g_Lara.left_arm.flash_gun = 0;
    g_Lara.left_arm.frame_number = AF_G_AIM;
    g_Lara.right_arm.x_rot = 0;
    g_Lara.right_arm.y_rot = 0;
    g_Lara.right_arm.z_rot = 0;
    g_Lara.right_arm.lock = 0;
    g_Lara.right_arm.flash_gun = 0;
    g_Lara.right_arm.frame_number = AF_G_AIM;
    g_Lara.target = NULL;

    switch (g_Lara.gun_type) {
    case LGT_PISTOLS:
    case LGT_MAGNUMS:
    case LGT_UZIS:
        g_Lara.right_arm.frame_base = g_Objects[O_PISTOLS].frame_base;
        g_Lara.left_arm.frame_base = g_Objects[O_PISTOLS].frame_base;
        if (g_Lara.gun_status != LGS_ARMLESS) {
            Gun_Pistols_DrawMeshes(g_Lara.gun_type);
        }
        break;

    case LGT_SHOTGUN:
        g_Lara.right_arm.frame_base = g_Objects[O_SHOTGUN].frame_base;
        g_Lara.left_arm.frame_base = g_Objects[O_SHOTGUN].frame_base;
        if (g_Lara.gun_status != LGS_ARMLESS) {
            Gun_Rifle_DrawMeshes();
        }
        break;

    default:
        g_Lara.right_arm.frame_base = g_Objects[O_LARA].frame_base;
        g_Lara.left_arm.frame_base = g_Objects[O_LARA].frame_base;
        break;
    }
}
