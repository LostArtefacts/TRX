#include "3dsystem/3d_gen.h"
#include "game/control.h"
#include "game/data.h"
#include "game/lara.h"
#include "game/misc.h"
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

void __cdecl LaraTargetInfo(WEAPON_INFO* winfo)
{
    if (!Lara.target) {
        Lara.right_arm.lock = 0;
        Lara.left_arm.lock = 0;
        Lara.target_angles[1] = 0;
        Lara.target_angles[0] = 0;
        return;
    }

    GAME_VECTOR src;
    GAME_VECTOR target;
    src.x = LaraItem->pos.x;
    src.y = LaraItem->pos.y - 650;
    src.z = LaraItem->pos.z;
    src.room_number = LaraItem->room_number;
    find_target_point(Lara.target, &target);

    int16_t ang[2];
    phd_GetVectorAngles(
        target.x - src.x, target.y - src.y, target.z - src.z, ang);
    ang[0] -= LaraItem->pos.y_rot;
    ang[1] -= LaraItem->pos.x_rot;

    if (LOS(&src, &target)) {
        if (ang[0] >= winfo->lock_angles[0] && ang[0] <= winfo->lock_angles[1]
            && ang[1] >= winfo->lock_angles[2]
            && ang[1] <= winfo->lock_angles[3]) {
            Lara.left_arm.lock = 1;
            Lara.right_arm.lock = 1;
        } else {
            if (Lara.left_arm.lock
                && (ang[0] < winfo->left_angles[0]
                    || ang[0] > winfo->left_angles[1]
                    || ang[1] < winfo->left_angles[2]
                    || ang[1] > winfo->left_angles[3])) {
                Lara.left_arm.lock = 0;
            }
            if (Lara.right_arm.lock
                && (ang[0] < winfo->right_angles[0]
                    || ang[0] > winfo->right_angles[1]
                    || ang[1] < winfo->right_angles[2]
                    || ang[1] > winfo->right_angles[3])) {
                Lara.right_arm.lock = 0;
            }
        }
    } else {
        Lara.right_arm.lock = 0;
        Lara.left_arm.lock = 0;
    }

    Lara.target_angles[0] = ang[0];
    Lara.target_angles[1] = ang[1];
}

void __cdecl LaraGetNewTarget(WEAPON_INFO* winfo)
{
    ITEM_INFO* bestitem = NULL;
    int16_t bestyrot = 0x7FFF;

    int32_t maxdist = winfo->target_dist;
    int32_t maxdist2 = maxdist * maxdist;
    GAME_VECTOR src;
    src.x = LaraItem->pos.x;
    src.y = LaraItem->pos.y - 650;
    src.z = LaraItem->pos.z;
    src.room_number = LaraItem->room_number;

    ITEM_INFO* item = NULL;
    for (int16_t item_num = NextItemActive; item_num != NO_ITEM;
         item_num = item->next_active) {
        item = &Items[item_num];
        if (item->hit_points <= 0) {
            continue;
        }

        int32_t x = item->pos.x - src.x;
        int32_t y = item->pos.y - src.y;
        int32_t z = item->pos.z - src.z;
        if (ABS(x) > maxdist || ABS(y) > maxdist || ABS(z) > maxdist) {
            continue;
        }

        int32_t dist = x * x + y * y + z * z;
        if (dist >= maxdist2) {
            continue;
        }

        GAME_VECTOR target;
        find_target_point(item, &target);
        if (!LOS(&src, &target)) {
            continue;
        }

        PHD_ANGLE ang[2];
        phd_GetVectorAngles(
            target.x - src.x, target.y - src.y, target.z - src.z, ang);
        ang[0] -= Lara.torso_y_rot + LaraItem->pos.y_rot;
        ang[1] -= Lara.torso_x_rot + LaraItem->pos.x_rot;
        if (ang[0] >= winfo->lock_angles[0] && ang[0] <= winfo->lock_angles[1]
            && ang[1] >= winfo->lock_angles[2]
            && ang[1] <= winfo->lock_angles[3]) {
            int16_t yrot = ABS(ang[0]);
            if (yrot < bestyrot) {
                bestyrot = yrot;
                bestitem = item;
            }
        }
    }

    Lara.target = bestitem;
    LaraTargetInfo(winfo);
}

void Tomb1MInjectGameLaraFire()
{
    INJECT(0x00426BD0, LaraGun);
    INJECT(0x00426E60, InitialiseNewWeapon);
    INJECT(0x00426F20, LaraTargetInfo);
    INJECT(0x004270C0, LaraGetNewTarget);
}
