#include "game/data.h"
#include "game/lara.h"
#include "util.h"

void __cdecl LaraGun()
{
    if (Lara.left_arm.flash_gun > 0) {
        --Lara.left_arm.flash_gun;
    }
    if (Lara.right_arm.flash_gun > 0) {
        --Lara.right_arm.flash_gun;
    }

    int32_t draw = 0;
    if (LaraItem->hit_points <= AF_G_AIM) {
        Lara.gun_status = LGS_ARMLESS;
    } else if (Lara.water_status == LWS_ABOVEWATER) {
        if (Lara.request_gun_type != Lara.gun_type) {
            if (Lara.gun_status == LGS_ARMLESS) {
                Lara.gun_type = Lara.request_gun_type;
                InitialiseNewWeapon();
                draw = 1;
            } else if (Lara.gun_status == LGS_READY) {
                draw = 1;
            }
        } else if (Input & IN_DRAW) {
            draw = 1;
        }
    } else if (Lara.gun_status == LGS_READY) {
        draw = 1;
    }

    if (draw && Lara.gun_type != LGT_UNARMED) {
        switch (Lara.gun_type) {
        case LGT_PISTOLS:
        case LGT_MAGNUMS:
        case LGT_UZIS:
            if (Lara.gun_status == LGS_ARMLESS) {
                Lara.gun_status = LGS_DRAW;
                Lara.right_arm.frame_number = AF_G_AIM;
                Lara.left_arm.frame_number = AF_G_AIM;
            } else if (Lara.gun_status == LGS_READY) {
                Lara.gun_status = LGS_UNDRAW;
            }
            break;

        case LGT_SHOTGUN:
            if (Lara.gun_status == LGS_ARMLESS) {
                Lara.gun_status = LGS_DRAW;
                Lara.left_arm.frame_number = AF_SG_AIM;
                Lara.right_arm.frame_number = AF_SG_AIM;
            } else if (Lara.gun_status == LGS_READY) {
                Lara.gun_status = LGS_UNDRAW;
            }
            break;
        }
    }

    switch (Lara.gun_status) {
    case LGS_DRAW:
        switch (Lara.gun_type) {
        case LGT_PISTOLS:
        case LGT_MAGNUMS:
        case LGT_UZIS:
            if (Camera.type != CAM_CINEMATIC && Camera.type != CAM_LOOK) {
                Camera.type = CAM_COMBAT;
            }
            draw_pistols(Lara.gun_type);
            break;

        case LGT_SHOTGUN:
            if (Camera.type != CAM_CINEMATIC && Camera.type != CAM_LOOK) {
                Camera.type = CAM_COMBAT;
            }
            draw_shotgun();
            break;
        }
        break;

    case LGS_UNDRAW:
        Lara.mesh_ptrs[LM_HEAD] = Meshes[Objects[O_LARA].mesh_index + LM_HEAD];
        switch (Lara.gun_type) {
        case LGT_PISTOLS:
        case LGT_MAGNUMS:
        case LGT_UZIS:
            undraw_pistols(Lara.gun_type);
            break;

        case LGT_SHOTGUN:
            undraw_shotgun();
            break;
        }
        break;

    case LGS_READY:
        Lara.mesh_ptrs[LM_HEAD] = Meshes[Objects[O_LARA].mesh_index + LM_HEAD];
        switch (Lara.gun_type) {
        case LGT_PISTOLS:
            if (Lara.pistols.ammo && (Input & IN_ACTION)) {
                Lara.mesh_ptrs[LM_HEAD] =
                    Meshes[Objects[O_UZI].mesh_index + LM_HEAD];
            }
            if (Camera.type != CAM_CINEMATIC && Camera.type != CAM_LOOK) {
                Camera.type = CAM_COMBAT;
            }
            PistolHandler(Lara.gun_type);
            break;

        case LGT_MAGNUMS:
            if (Lara.magnums.ammo && (Input & IN_ACTION)) {
                Lara.mesh_ptrs[LM_HEAD] =
                    Meshes[Objects[O_UZI].mesh_index + LM_HEAD];
            }
            if (Camera.type != CAM_CINEMATIC && Camera.type != CAM_LOOK) {
                Camera.type = CAM_COMBAT;
            }
            PistolHandler(Lara.gun_type);
            break;

        case LGT_UZIS:
            if (Lara.uzis.ammo && (Input & IN_ACTION)) {
                Lara.mesh_ptrs[LM_HEAD] =
                    Meshes[Objects[O_UZI].mesh_index + LM_HEAD];
            }
            if (Camera.type != CAM_CINEMATIC && Camera.type != CAM_LOOK) {
                Camera.type = CAM_COMBAT;
            }
            PistolHandler(Lara.gun_type);
            break;

        case LGT_SHOTGUN:
            if (Lara.shotgun.ammo && (Input & IN_ACTION)) {
                Lara.mesh_ptrs[LM_HEAD] =
                    Meshes[Objects[O_UZI].mesh_index + LM_HEAD];
            }
            if (Camera.type != CAM_CINEMATIC && Camera.type != CAM_LOOK) {
                Camera.type = CAM_COMBAT;
            }
            RifleHandler(LGT_SHOTGUN);
            break;
        }
        break;
    }
}

void __cdecl InitialiseNewWeapon()
{
    Lara.left_arm.x_rot = 0;
    Lara.left_arm.y_rot = 0;
    Lara.left_arm.z_rot = 0;
    Lara.left_arm.lock = 0;
    Lara.left_arm.flash_gun = 0;
    Lara.left_arm.frame_number = AF_G_AIM;
    Lara.right_arm.x_rot = 0;
    Lara.right_arm.y_rot = 0;
    Lara.right_arm.z_rot = 0;
    Lara.right_arm.lock = 0;
    Lara.right_arm.flash_gun = 0;
    Lara.right_arm.frame_number = AF_G_AIM;
    Lara.target = NULL;

    switch (Lara.gun_type) {
    case LGT_PISTOLS:
    case LGT_MAGNUMS:
    case LGT_UZIS:
        Lara.right_arm.frame_base = Objects[O_PISTOLS].frame_base;
        Lara.left_arm.frame_base = Objects[O_PISTOLS].frame_base;
        if (Lara.gun_status != LGS_ARMLESS) {
            draw_pistol_meshes(Lara.gun_type);
        }
        break;

    case LGT_SHOTGUN:
        Lara.right_arm.frame_base = Objects[O_SHOTGUN].frame_base;
        Lara.left_arm.frame_base = Objects[O_SHOTGUN].frame_base;
        if (Lara.gun_status != LGS_ARMLESS) {
            draw_shotgun_meshes();
        }
        break;

    default:
        Lara.right_arm.frame_base = Objects[O_LARA].frame_base;
        Lara.left_arm.frame_base = Objects[O_LARA].frame_base;
        break;
    }
}

void Tomb1MInjectGameLaraFire()
{
    INJECT(0x00426BD0, LaraGun);
    INJECT(0x00426E60, InitialiseNewWeapon);
}
