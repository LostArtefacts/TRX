#include "game/lara.h"

#include "3dsystem/3d_gen.h"
#include "3dsystem/matrix.h"
#include "3dsystem/phd_math.h"
#include "config.h"
#include "game/control.h"
#include "game/draw.h"
#include "game/effects/blood.h"
#include "game/effects/ricochet.h"
#include "game/input.h"
#include "game/inv.h"
#include "game/random.h"
#include "game/sound.h"
#include "game/sphere.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <stddef.h>
#include <stdint.h>

#define PISTOL_LOCK_YMIN (-60 * PHD_DEGREE)
#define PISTOL_LOCK_YMAX (+60 * PHD_DEGREE)
#define PISTOL_LOCK_XMIN (-60 * PHD_DEGREE)
#define PISTOL_LOCK_XMAX (+60 * PHD_DEGREE)

#define PISTOL_LARM_YMIN (-170 * PHD_DEGREE)
#define PISTOL_LARM_YMAX (+60 * PHD_DEGREE)
#define PISTOL_LARM_XMIN (-80 * PHD_DEGREE)
#define PISTOL_LARM_XMAX (+80 * PHD_DEGREE)

#define PISTOL_RARM_YMIN (-60 * PHD_DEGREE)
#define PISTOL_RARM_YMAX (+170 * PHD_DEGREE)
#define PISTOL_RARM_XMIN (-80 * PHD_DEGREE)
#define PISTOL_RARM_XMAX (+80 * PHD_DEGREE)

#define SHOTGUN_LOCK_YMIN (-60 * PHD_DEGREE)
#define SHOTGUN_LOCK_YMAX (+60 * PHD_DEGREE)
#define SHOTGUN_LOCK_XMIN (-55 * PHD_DEGREE)
#define SHOTGUN_LOCK_XMAX (+55 * PHD_DEGREE)

#define SHOTGUN_LARM_YMIN (-80 * PHD_DEGREE)
#define SHOTGUN_LARM_YMAX (+80 * PHD_DEGREE)
#define SHOTGUN_LARM_XMIN (-65 * PHD_DEGREE)
#define SHOTGUN_LARM_XMAX (+65 * PHD_DEGREE)

#define SHOTGUN_RARM_YMIN (-80 * PHD_DEGREE)
#define SHOTGUN_RARM_YMAX (+80 * PHD_DEGREE)
#define SHOTGUN_RARM_XMIN (-65 * PHD_DEGREE)
#define SHOTGUN_RARM_XMAX (+65 * PHD_DEGREE)

WEAPON_INFO g_Weapons[NUM_WEAPONS] = {
    // null
    {
        { 0, 0, 0, 0 }, // lock_angles
        { 0, 0, 0, 0 }, // left_angles
        { 0, 0, 0, 0 }, // right_angles
        0, // aim_speed
        0, // shot_accuracy
        0, // gun_height
        0, // damage
        0, // target_dist
        0, // recoil_frame
        0, // flash_time
        SFX_LARA_NO, // sample_num
    },

    // pistols
    {
        { PISTOL_LOCK_YMIN, PISTOL_LOCK_YMAX, PISTOL_LOCK_XMIN,
          PISTOL_LOCK_XMAX }, // lock_angles
        { PISTOL_LARM_YMIN, PISTOL_LARM_YMAX, PISTOL_LARM_XMIN,
          PISTOL_LARM_XMAX }, // left_angles
        { PISTOL_RARM_YMIN, PISTOL_RARM_YMAX, PISTOL_RARM_XMIN,
          PISTOL_RARM_XMAX }, // right_angles
        10 * PHD_DEGREE, // aim_speed
        8 * PHD_DEGREE, // shot_accuracy
        650, // gun_height
        1, // damage
        8 * WALL_L, // target_dist
        9, // recoil_frame
        3, // flash_time
        SFX_LARA_FIRE, // sample_num
    },

    // magnums
    {
        { PISTOL_LOCK_YMIN, PISTOL_LOCK_YMAX, PISTOL_LOCK_XMIN,
          PISTOL_LOCK_XMAX }, // lock_angles
        { PISTOL_LARM_YMIN, PISTOL_LARM_YMAX, PISTOL_LARM_XMIN,
          PISTOL_LARM_XMAX }, // left_angles
        { PISTOL_RARM_YMIN, PISTOL_RARM_YMAX, PISTOL_RARM_XMIN,
          PISTOL_RARM_XMAX }, // right_angles
        10 * PHD_DEGREE, // aim_speed
        8 * PHD_DEGREE, // shot_accuracy
        650, // gun_height
        2, // damage
        8 * WALL_L, // target_dist
        9, // recoil_frame
        3, // flash_time
        SFX_LARA_MAGNUMS, // sample_num
    },

    // uzis
    {
        { PISTOL_LOCK_YMIN, PISTOL_LOCK_YMAX, PISTOL_LOCK_XMIN,
          PISTOL_LOCK_XMAX }, // lock_angles
        { PISTOL_LARM_YMIN, PISTOL_LARM_YMAX, PISTOL_LARM_XMIN,
          PISTOL_LARM_XMAX }, // left_angles
        { PISTOL_RARM_YMIN, PISTOL_RARM_YMAX, PISTOL_RARM_XMIN,
          PISTOL_RARM_XMAX }, // right_angles
        10 * PHD_DEGREE, // aim_speed
        8 * PHD_DEGREE, // shot_accuracy
        650, // gun_height
        1, // damage
        8 * WALL_L, // target_dist
        3, // recoil_frame
        2, // flash_time
        SFX_LARA_UZI_FIRE, // sample_num
    },

    // shotgun
    {
        { SHOTGUN_LOCK_YMIN, SHOTGUN_LOCK_YMAX, SHOTGUN_LOCK_XMIN,
          SHOTGUN_LOCK_XMAX }, // lock_angles
        { SHOTGUN_LARM_YMIN, SHOTGUN_LARM_YMAX, SHOTGUN_LARM_XMIN,
          SHOTGUN_LARM_XMAX }, // left_angles
        { SHOTGUN_RARM_YMIN, SHOTGUN_RARM_YMAX, SHOTGUN_RARM_XMIN,
          SHOTGUN_RARM_XMAX }, // right_angles
        10 * PHD_DEGREE, // aim_speed
        0, // shot_accuracy
        0x1F4, // gun_height
        4, // damage
        8 * WALL_L, // target_dist
        9, // recoil_frame
        3, // flash_time
        SFX_LARA_SHOTGUN, // sample_num
    },
};

void LaraGun(void)
{
    if (g_Lara.left_arm.flash_gun > 0) {
        g_Lara.left_arm.flash_gun--;
    }
    if (g_Lara.right_arm.flash_gun > 0) {
        g_Lara.right_arm.flash_gun--;
    }

    int32_t draw = 0;
    if (g_LaraItem->hit_points <= 0) {
        g_Lara.gun_status = LGS_ARMLESS;
    } else if (g_Lara.water_status == LWS_ABOVEWATER) {
        if (g_Lara.request_gun_type != LGT_UNARMED
            && (g_Lara.request_gun_type != g_Lara.gun_type
                || g_Lara.gun_status == LGS_ARMLESS)) {
            if (g_Lara.gun_status == LGS_ARMLESS) {
                g_Lara.gun_type = g_Lara.request_gun_type;
                InitialiseNewWeapon();
                draw = 1;
                g_Lara.request_gun_type = LGT_UNARMED;
            } else if (g_Lara.gun_status == LGS_READY) {
                draw = 1;
            }
        } else if (g_Input.draw) {
            if (g_Lara.gun_type == LGT_UNARMED && Inv_RequestItem(O_GUN_ITEM)) {
                g_Lara.gun_type = LGT_PISTOLS;
                InitialiseNewWeapon();
            }
            draw = 1;
            g_Lara.request_gun_type = LGT_UNARMED;
        }
    } else if (g_Lara.gun_status == LGS_READY) {
        draw = 1;
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
            DrawPistols(g_Lara.gun_type);
            break;

        case LGT_SHOTGUN:
            if (g_Camera.type != CAM_CINEMATIC && g_Camera.type != CAM_LOOK) {
                g_Camera.type = CAM_COMBAT;
            }
            DrawShotgun();
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
            UndrawPistols(g_Lara.gun_type);
            break;

        case LGT_SHOTGUN:
            UndrawShotgun();
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
            PistolHandler(g_Lara.gun_type);
            break;

        case LGT_MAGNUMS:
            if (g_Lara.magnums.ammo && g_Input.action) {
                g_Lara.mesh_ptrs[LM_HEAD] =
                    g_Meshes[g_Objects[O_UZI].mesh_index + LM_HEAD];
            }
            if (g_Camera.type != CAM_CINEMATIC && g_Camera.type != CAM_LOOK) {
                g_Camera.type = CAM_COMBAT;
            }
            PistolHandler(g_Lara.gun_type);
            break;

        case LGT_UZIS:
            if (g_Lara.uzis.ammo && g_Input.action) {
                g_Lara.mesh_ptrs[LM_HEAD] =
                    g_Meshes[g_Objects[O_UZI].mesh_index + LM_HEAD];
            }
            if (g_Camera.type != CAM_CINEMATIC && g_Camera.type != CAM_LOOK) {
                g_Camera.type = CAM_COMBAT;
            }
            PistolHandler(g_Lara.gun_type);
            break;

        case LGT_SHOTGUN:
            if (g_Lara.shotgun.ammo && g_Input.action) {
                g_Lara.mesh_ptrs[LM_HEAD] =
                    g_Meshes[g_Objects[O_UZI].mesh_index + LM_HEAD];
            }
            if (g_Camera.type != CAM_CINEMATIC && g_Camera.type != CAM_LOOK) {
                g_Camera.type = CAM_COMBAT;
            }
            RifleHandler(LGT_SHOTGUN);
            break;
        }
        break;
    }
}

void InitialiseNewWeapon(void)
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
            DrawPistolMeshes(g_Lara.gun_type);
        }
        break;

    case LGT_SHOTGUN:
        g_Lara.right_arm.frame_base = g_Objects[O_SHOTGUN].frame_base;
        g_Lara.left_arm.frame_base = g_Objects[O_SHOTGUN].frame_base;
        if (g_Lara.gun_status != LGS_ARMLESS) {
            DrawShotgunMeshes();
        }
        break;

    default:
        g_Lara.right_arm.frame_base = g_Objects[O_LARA].frame_base;
        g_Lara.left_arm.frame_base = g_Objects[O_LARA].frame_base;
        break;
    }
}

void LaraTargetInfo(WEAPON_INFO *winfo)
{
    if (!g_Lara.target) {
        g_Lara.right_arm.lock = 0;
        g_Lara.left_arm.lock = 0;
        g_Lara.target_angles[1] = 0;
        g_Lara.target_angles[0] = 0;
        return;
    }

    GAME_VECTOR src;
    GAME_VECTOR target;
    src.x = g_LaraItem->pos.x;
    src.y = g_LaraItem->pos.y - 650;
    src.z = g_LaraItem->pos.z;
    src.room_number = g_LaraItem->room_number;
    find_target_point(g_Lara.target, &target);

    int16_t ang[2];
    phd_GetVectorAngles(
        target.x - src.x, target.y - src.y, target.z - src.z, ang);
    ang[0] -= g_LaraItem->pos.y_rot;
    ang[1] -= g_LaraItem->pos.x_rot;

    if (LOS(&src, &target)) {
        if (ang[0] >= winfo->lock_angles[0] && ang[0] <= winfo->lock_angles[1]
            && ang[1] >= winfo->lock_angles[2]
            && ang[1] <= winfo->lock_angles[3]) {
            g_Lara.left_arm.lock = 1;
            g_Lara.right_arm.lock = 1;
        } else {
            if (g_Lara.left_arm.lock
                && (ang[0] < winfo->left_angles[0]
                    || ang[0] > winfo->left_angles[1]
                    || ang[1] < winfo->left_angles[2]
                    || ang[1] > winfo->left_angles[3])) {
                g_Lara.left_arm.lock = 0;
            }
            if (g_Lara.right_arm.lock
                && (ang[0] < winfo->right_angles[0]
                    || ang[0] > winfo->right_angles[1]
                    || ang[1] < winfo->right_angles[2]
                    || ang[1] > winfo->right_angles[3])) {
                g_Lara.right_arm.lock = 0;
            }
        }
    } else {
        g_Lara.right_arm.lock = 0;
        g_Lara.left_arm.lock = 0;
    }

    g_Lara.target_angles[0] = ang[0];
    g_Lara.target_angles[1] = ang[1];
}

void LaraGetNewTarget(WEAPON_INFO *winfo)
{
    ITEM_INFO *bestitem = NULL;
    int16_t bestyrot = 0x7FFF;

    int32_t maxdist = winfo->target_dist;
    int32_t maxdist2 = maxdist * maxdist;
    GAME_VECTOR src;
    src.x = g_LaraItem->pos.x;
    src.y = g_LaraItem->pos.y - 650;
    src.z = g_LaraItem->pos.z;
    src.room_number = g_LaraItem->room_number;

    ITEM_INFO *item = NULL;
    for (int16_t item_num = g_NextItemActive; item_num != NO_ITEM;
         item_num = item->next_active) {
        item = &g_Items[item_num];
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
        ang[0] -= g_Lara.torso_y_rot + g_LaraItem->pos.y_rot;
        ang[1] -= g_Lara.torso_x_rot + g_LaraItem->pos.x_rot;
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

    g_Lara.target = bestitem;
    LaraTargetInfo(winfo);
}

void find_target_point(ITEM_INFO *item, GAME_VECTOR *target)
{
    int16_t *bounds = GetBestFrame(item);
    int32_t x = (bounds[0] + bounds[1]) / 2;
    int32_t y = (bounds[3] - bounds[2]) / 3 + bounds[2];
    int32_t z = (bounds[5] + bounds[4]) / 2;
    int32_t c = phd_cos(item->pos.y_rot);
    int32_t s = phd_sin(item->pos.y_rot);
    target->x = item->pos.x + ((c * x + s * z) >> W2V_SHIFT);
    target->y = item->pos.y + y;
    target->z = item->pos.z + ((c * z - s * x) >> W2V_SHIFT);
    target->room_number = item->room_number;
}

void AimWeapon(WEAPON_INFO *winfo, LARA_ARM *arm)
{
    PHD_ANGLE destx;
    PHD_ANGLE desty;
    PHD_ANGLE curr;
    PHD_ANGLE speed = winfo->aim_speed;

    if (arm->lock) {
        desty = g_Lara.target_angles[0];
        destx = g_Lara.target_angles[1];
    } else {
        destx = 0;
        desty = 0;
    }

    curr = arm->y_rot;
    if (curr >= desty - speed && curr <= speed + desty) {
        curr = desty;
    } else if (curr < desty) {
        curr += speed;
    } else {
        curr -= speed;
    }
    arm->y_rot = curr;

    curr = arm->x_rot;
    if (curr >= destx - speed && curr <= speed + destx) {
        curr = destx;
    } else if (curr < destx) {
        curr += speed;
    } else {
        curr -= speed;
    }
    arm->x_rot = curr;

    arm->z_rot = 0;
}

int32_t FireWeapon(
    int32_t weapon_type, ITEM_INFO *target, ITEM_INFO *src, PHD_ANGLE *angles)
{
    WEAPON_INFO *winfo = &g_Weapons[weapon_type];

    AMMO_INFO *ammo;
    switch (weapon_type) {
    case LGT_MAGNUMS:
        ammo = &g_Lara.magnums;
        if (g_GameInfo.bonus_flag & GBF_NGPLUS) {
            ammo->ammo = 1000;
        }
        break;

    case LGT_UZIS:
        ammo = &g_Lara.uzis;
        if (g_GameInfo.bonus_flag & GBF_NGPLUS) {
            ammo->ammo = 1000;
        }
        break;

    case LGT_SHOTGUN:
        ammo = &g_Lara.shotgun;
        if (g_GameInfo.bonus_flag & GBF_NGPLUS) {
            ammo->ammo = 1000;
        }
        break;

    default:
        ammo = &g_Lara.pistols;
        ammo->ammo = 1000;
        break;
    }

    if (ammo->ammo <= 0) {
        ammo->ammo = 0;
        Sound_Effect(SFX_LARA_EMPTY, &src->pos, SPM_NORMAL);
        if (Inv_RequestItem(O_GUN_ITEM)) {
            g_Lara.request_gun_type = LGT_PISTOLS;
        }
        return 0;
    }

    ammo->ammo--;

    PHD_3DPOS view;
    view.x = src->pos.x;
    view.y = src->pos.y - winfo->gun_height;
    view.z = src->pos.z;
    view.x_rot = angles[1]
        + (winfo->shot_accuracy * (Random_GetControl() - PHD_90)) / PHD_ONE;
    view.y_rot = angles[0]
        + (winfo->shot_accuracy * (Random_GetControl() - PHD_90)) / PHD_ONE;
    view.z_rot = 0;
    phd_GenerateW2V(&view);

    SPHERE slist[33];
    int32_t nums = GetSpheres(target, slist, 0);

    int32_t best = -1;
    int32_t bestdist = 0x7FFFFFFF;
    for (int i = 0; i < nums; i++) {
        SPHERE *sptr = &slist[i];
        int32_t r = sptr->r;
        if (ABS(sptr->x) < r && ABS(sptr->y) < r && sptr->z > r
            && (sptr->x * sptr->x) + (sptr->y * sptr->y) <= (r * r)
            && (sptr->z - r < bestdist)) {
            bestdist = sptr->z - r;
            best = i;
        }
    }

    GAME_VECTOR vsrc;
    vsrc.room_number = src->room_number;
    vsrc.x = view.x;
    vsrc.y = view.y;
    vsrc.z = view.z;

    GAME_VECTOR vdest;
    if (best >= 0) {
        ammo->hit++;
        vdest.x = view.x + ((bestdist * g_PhdMatrixPtr->_20) >> W2V_SHIFT);
        vdest.y = view.y + ((bestdist * g_PhdMatrixPtr->_21) >> W2V_SHIFT);
        vdest.z = view.z + ((bestdist * g_PhdMatrixPtr->_22) >> W2V_SHIFT);
        HitTarget(
            target, &vdest,
            winfo->damage * (g_GameInfo.bonus_flag & GBF_JAPANESE ? 2 : 1));
        return 1;
    }

    ammo->miss++;
    vdest.x = vsrc.x + g_PhdMatrixPtr->_20;
    vdest.y = vsrc.y + g_PhdMatrixPtr->_21;
    vdest.z = vsrc.z + g_PhdMatrixPtr->_22;
    LOS(&vsrc, &vdest);
    Ricochet(&vdest);
    return -1;
}

void HitTarget(ITEM_INFO *item, GAME_VECTOR *hitpos, int32_t damage)
{
    if (item->hit_points > 0 && item->hit_points <= damage) {
        g_GameInfo.stats.kill_count++;
    }
    item->hit_points -= damage;
    item->hit_status = 1;

    Blood_Spawn(
        hitpos->x, hitpos->y, hitpos->z, item->speed, item->pos.y_rot,
        item->room_number);

    if (item->hit_points > 0) {
        switch (item->object_number) {
        case O_WOLF:
            Sound_Effect(SFX_WOLF_HURT, &item->pos, SPM_NORMAL);
            break;

        case O_BEAR:
            Sound_Effect(SFX_BEAR_HURT, &item->pos, SPM_NORMAL);
            break;

        case O_LION:
        case O_LIONESS:
            Sound_Effect(SFX_LION_HURT, &item->pos, SPM_NORMAL);
            break;

        case O_RAT:
            Sound_Effect(SFX_RAT_CHIRP, &item->pos, SPM_NORMAL);
            break;

        case O_SKATEKID:
            Sound_Effect(SFX_SKATEBOARD_HIT, &item->pos, SPM_NORMAL);
            break;

        case O_ABORTION:
            Sound_Effect(SFX_ABORTION_HIT, &item->pos, SPM_NORMAL);
            break;

        default:
            break;
        }
    }
}
