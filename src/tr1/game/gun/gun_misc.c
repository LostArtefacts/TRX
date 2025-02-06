#include "game/gun/gun_misc.h"

#include "game/collide.h"
#include "game/game.h"
#include "game/input.h"
#include "game/inventory.h"
#include "game/items.h"
#include "game/los.h"
#include "game/random.h"
#include "game/savegame.h"
#include "game/sound.h"
#include "game/spawn.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/math.h>
#include <libtrx/game/matrix.h>
#include <libtrx/utils.h>

#define PISTOL_LOCK_YMIN (-60 * DEG_1)
#define PISTOL_LOCK_YMAX (+60 * DEG_1)
#define PISTOL_LOCK_XMIN (-60 * DEG_1)
#define PISTOL_LOCK_XMAX (+60 * DEG_1)

#define PISTOL_LARM_YMIN (-170 * DEG_1)
#define PISTOL_LARM_YMAX (+60 * DEG_1)
#define PISTOL_LARM_XMIN (-80 * DEG_1)
#define PISTOL_LARM_XMAX (+80 * DEG_1)

#define PISTOL_RARM_YMIN (-60 * DEG_1)
#define PISTOL_RARM_YMAX (+170 * DEG_1)
#define PISTOL_RARM_XMIN (-80 * DEG_1)
#define PISTOL_RARM_XMAX (+80 * DEG_1)

#define SHOTGUN_LOCK_YMIN (-60 * DEG_1)
#define SHOTGUN_LOCK_YMAX (+60 * DEG_1)
#define SHOTGUN_LOCK_XMIN (-55 * DEG_1)
#define SHOTGUN_LOCK_XMAX (+55 * DEG_1)

#define SHOTGUN_LARM_YMIN (-80 * DEG_1)
#define SHOTGUN_LARM_YMAX (+80 * DEG_1)
#define SHOTGUN_LARM_XMIN (-65 * DEG_1)
#define SHOTGUN_LARM_XMAX (+65 * DEG_1)

#define SHOTGUN_RARM_YMIN (-80 * DEG_1)
#define SHOTGUN_RARM_YMAX (+80 * DEG_1)
#define SHOTGUN_RARM_XMIN (-65 * DEG_1)
#define SHOTGUN_RARM_XMAX (+65 * DEG_1)

static ITEM *m_TargetList[NUM_SLOTS];
static ITEM *m_LastTargetList[NUM_SLOTS];

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
        10 * DEG_1, // aim_speed
        8 * DEG_1, // shot_accuracy
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
        10 * DEG_1, // aim_speed
        8 * DEG_1, // shot_accuracy
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
        10 * DEG_1, // aim_speed
        8 * DEG_1, // shot_accuracy
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
        10 * DEG_1, // aim_speed
        0, // shot_accuracy
        0x1F4, // gun_height
        4, // damage
        8 * WALL_L, // target_dist
        9, // recoil_frame
        3, // flash_time
        SFX_LARA_SHOTGUN, // sample_num
    },
};

void Gun_TargetInfo(WEAPON_INFO *winfo)
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
    src.room_num = g_LaraItem->room_num;
    Gun_FindTargetPoint(g_Lara.target, &target);

    int16_t ang[2];
    Math_GetVectorAngles(
        target.x - src.x, target.y - src.y, target.z - src.z, ang);
    ang[0] -= g_LaraItem->rot.y;
    ang[1] -= g_LaraItem->rot.x;

    if (LOS_Check(&src, &target)) {
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

void Gun_GetNewTarget(WEAPON_INFO *winfo)
{
    // Preserve OG targeting behavior.
    if (g_Config.gameplay.target_mode == TLM_FULL
        && !g_Config.gameplay.enable_target_change && !g_Input.action) {
        g_Lara.target = nullptr;
    }

    ITEM *best_target = nullptr;
    int16_t best_yrot = 0x7FFF;
    int16_t num_targets = 0;

    int32_t maxdist = winfo->target_dist;
    int32_t maxdist2 = maxdist * maxdist;
    GAME_VECTOR src;
    src.x = g_LaraItem->pos.x;
    src.y = g_LaraItem->pos.y - 650;
    src.z = g_LaraItem->pos.z;
    src.room_num = g_LaraItem->room_num;

    int16_t item_num = Item_GetNextActive();
    while (item_num != NO_ITEM) {
        ITEM *const item = Item_Get(item_num);
        item_num = item->next_active;
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
        Gun_FindTargetPoint(item, &target);
        if (!LOS_Check(&src, &target)) {
            continue;
        }

        PHD_ANGLE ang[2];
        Math_GetVectorAngles(
            target.x - src.x, target.y - src.y, target.z - src.z, ang);
        ang[0] -= g_Lara.torso_rot.y + g_LaraItem->rot.y;
        ang[1] -= g_Lara.torso_rot.x + g_LaraItem->rot.x;
        if (ang[0] >= winfo->lock_angles[0] && ang[0] <= winfo->lock_angles[1]
            && ang[1] >= winfo->lock_angles[2]
            && ang[1] <= winfo->lock_angles[3]) {
            int16_t yrot = ABS(ang[0]);
            m_TargetList[num_targets] = item;
            num_targets++;
            if (yrot < best_yrot) {
                best_yrot = yrot;
                best_target = item;
            }
        }
    }
    m_TargetList[num_targets] = nullptr;

    if ((g_Config.gameplay.target_mode == TLM_FULL
         || g_Config.gameplay.target_mode == TLM_SEMI)
        && g_Input.action && g_Lara.target) {
        Gun_TargetInfo(winfo);
        return;
    }

    if (num_targets > 0) {
        for (int slot = 0; slot < NUM_SLOTS; slot++) {
            if (!m_TargetList[slot]) {
                g_Lara.target = nullptr;
            }

            if (m_TargetList[slot] == g_Lara.target) {
                break;
            }
        }

        if (!g_Lara.target) {
            g_Lara.target = best_target;
            m_LastTargetList[0] = nullptr;
        }
    } else {
        g_Lara.target = nullptr;
    }

    if (g_Lara.target != m_LastTargetList[0]) {
        for (int slot = NUM_SLOTS - 1; slot > 0; slot--) {
            m_LastTargetList[slot] = m_LastTargetList[slot - 1];
        }
        m_LastTargetList[0] = g_Lara.target;
    }

    Gun_TargetInfo(winfo);
}

void Gun_ChangeTarget(WEAPON_INFO *winfo)
{
    g_Lara.target = nullptr;
    bool found_new_target = false;

    for (int new_target = 0; new_target < NUM_SLOTS; new_target++) {
        if (!m_TargetList[new_target]) {
            break;
        }

        for (int last_target = 0; last_target < NUM_SLOTS; last_target++) {
            if (!m_LastTargetList[last_target]) {
                found_new_target = true;
                break;
            }

            if (m_LastTargetList[last_target] == m_TargetList[new_target]) {
                break;
            }
        }

        if (found_new_target) {
            g_Lara.target = m_TargetList[new_target];
            break;
        }
    }

    if (g_Lara.target != m_LastTargetList[0]) {
        for (int last_target = NUM_SLOTS - 1; last_target > 0; last_target--) {
            m_LastTargetList[last_target] = m_LastTargetList[last_target - 1];
        }
        m_LastTargetList[0] = g_Lara.target;
    }

    Gun_TargetInfo(winfo);
}

void Gun_FindTargetPoint(ITEM *item, GAME_VECTOR *target)
{
    const BOUNDS_16 *const bounds = &Item_GetBestFrame(item)->bounds;
    const int32_t x = (bounds->min.x + bounds->max.x) / 2;
    const int32_t y = (bounds->max.y - bounds->min.y) / 3 + bounds->min.y;
    const int32_t z = (bounds->min.z + bounds->max.z) / 2;
    const int32_t c = Math_Cos(item->rot.y);
    const int32_t s = Math_Sin(item->rot.y);
    target->x = item->pos.x + ((c * x + s * z) >> W2V_SHIFT);
    target->y = item->pos.y + y;
    target->z = item->pos.z + ((c * z - s * x) >> W2V_SHIFT);
    target->room_num = item->room_num;
}

void Gun_AimWeapon(WEAPON_INFO *winfo, LARA_ARM *arm)
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

    curr = arm->rot.y;
    if (curr >= desty - speed && curr <= speed + desty) {
        curr = desty;
    } else if (curr < desty) {
        curr += speed;
    } else {
        curr -= speed;
    }
    arm->rot.y = curr;

    curr = arm->rot.x;
    if (curr >= destx - speed && curr <= speed + destx) {
        curr = destx;
    } else if (curr < destx) {
        curr += speed;
    } else {
        curr -= speed;
    }
    arm->rot.x = curr;

    arm->rot.z = 0;
}

int32_t Gun_FireWeapon(
    int32_t weapon_type, ITEM *target, ITEM *src, PHD_ANGLE *angles)
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
        if (Inv_RequestItem(O_PISTOL_ITEM)) {
            g_Lara.request_gun_type = LGT_PISTOLS;
        } else {
            g_Lara.gun_status = LGS_UNDRAW;
        }
        return 0;
    }

    ammo->ammo--;

    const XYZ_32 view_pos = {
        .x = src->pos.x,
        .y = src->pos.y - winfo->gun_height,
        .z = src->pos.z,
    };
    const XYZ_16 view_rot = {
        .x = angles[1]
            + (winfo->shot_accuracy * (Random_GetControl() - DEG_90)) / DEG_360,
        .y = angles[0]
            + (winfo->shot_accuracy * (Random_GetControl() - DEG_90)) / DEG_360,
        .z = 0,
    };
    Matrix_GenerateW2V(&view_pos, &view_rot);

    SPHERE slist[33];
    int32_t nums = Collide_GetSpheres(target, slist, 0);

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
    vsrc.room_num = src->room_num;
    vsrc.pos = view_pos;

    GAME_VECTOR vdest;
    if (best >= 0) {
        ammo->hit++;
        vdest.x = vsrc.x + ((bestdist * g_MatrixPtr->_20) >> W2V_SHIFT);
        vdest.y = vsrc.y + ((bestdist * g_MatrixPtr->_21) >> W2V_SHIFT);
        vdest.z = vsrc.z + ((bestdist * g_MatrixPtr->_22) >> W2V_SHIFT);
        Gun_HitTarget(
            target, &vdest,
            winfo->damage * (g_GameInfo.bonus_flag & GBF_JAPANESE ? 2 : 1));
        return 1;
    }

    ammo->miss++;
    vdest.x = vsrc.x + g_MatrixPtr->_20;
    vdest.y = vsrc.y + g_MatrixPtr->_21;
    vdest.z = vsrc.z + g_MatrixPtr->_22;
    LOS_Check(&vsrc, &vdest);
    Spawn_Ricochet(&vdest);
    return -1;
}

void Gun_HitTarget(ITEM *item, GAME_VECTOR *hitpos, int16_t damage)
{
    if (item->hit_points > 0 && item->hit_points <= damage) {
        Savegame_GetCurrentInfo(Game_GetCurrentLevel())->stats.kill_count++;
        if (g_Config.gameplay.target_mode == TLM_SEMI) {
            g_Lara.target = nullptr;
        }
    }
    Item_TakeDamage(item, damage, true);

    if (g_Config.visuals.fix_texture_issues
        && item->object_id == O_SCION_ITEM_3) {
        GAME_VECTOR pos;
        pos.x = hitpos->x;
        pos.y = hitpos->y;
        pos.z = hitpos->z;
        pos.room_num = item->room_num;
        Spawn_Ricochet(&pos);
    } else {
        Spawn_Blood(
            hitpos->x, hitpos->y, hitpos->z, item->speed, item->rot.y,
            item->room_num);
    }

    if (item->hit_points > 0) {
        switch (item->object_id) {
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

        case O_TORSO:
            Sound_Effect(SFX_TORSO_HIT, &item->pos, SPM_NORMAL);
            break;

        default:
            break;
        }
    }
}
