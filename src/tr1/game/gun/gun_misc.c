#include "game/gun/gun_misc.h"

#include "game/game.h"
#include "game/inventory.h"
#include "game/los.h"
#include "game/savegame.h"
#include "game/spawn.h"
#include "game/stats.h"

#include <libtrx/config.h>
#include <libtrx/game/collision.h>
#include <libtrx/game/gun/vars.h>
#include <libtrx/game/input.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/math.h>
#include <libtrx/game/matrix.h>
#include <libtrx/game/random.h>
#include <libtrx/game/sound.h>
#include <libtrx/utils.h>

static ITEM *m_TargetList[LOT_SLOT_COUNT];
static ITEM *m_LastTargetList[LOT_SLOT_COUNT];

void Gun_GetNewTarget(WEAPON_INFO *const weapon)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    const ITEM *const lara_item = Lara_GetItem();

    // Preserve OG targeting behavior.
    if (g_Config.gameplay.target_mode == TLM_FULL
        && !g_Config.gameplay.enable_target_change && !g_Input.action) {
        lara->target = nullptr;
    }

    ITEM *best_target = nullptr;
    int16_t best_yrot = 0x7FFF;
    int16_t num_targets = 0;

    int32_t maxdist = weapon->target_dist;
    int32_t maxdist2 = maxdist * maxdist;
    GAME_VECTOR src;
    src.x = lara_item->pos.x;
    src.y = lara_item->pos.y - 650;
    src.z = lara_item->pos.z;
    src.room_num = lara_item->room_num;

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
        ang[0] -= lara->torso_rot.y + lara_item->rot.y;
        ang[1] -= lara->torso_rot.x + lara_item->rot.x;
        if (ang[0] >= weapon->lock_angles[0] && ang[0] <= weapon->lock_angles[1]
            && ang[1] >= weapon->lock_angles[2]
            && ang[1] <= weapon->lock_angles[3]) {
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
        && g_Input.action && lara->target != nullptr) {
        Gun_TargetInfo(weapon);
        return;
    }

    if (num_targets > 0) {
        for (int slot = 0; slot < LOT_SLOT_COUNT; slot++) {
            if (!m_TargetList[slot]) {
                lara->target = nullptr;
            }

            if (m_TargetList[slot] == lara->target) {
                break;
            }
        }

        if (lara->target == nullptr) {
            lara->target = best_target;
            m_LastTargetList[0] = nullptr;
        }
    } else {
        lara->target = nullptr;
    }

    if (lara->target != m_LastTargetList[0]) {
        for (int slot = LOT_SLOT_COUNT - 1; slot > 0; slot--) {
            m_LastTargetList[slot] = m_LastTargetList[slot - 1];
        }
        m_LastTargetList[0] = lara->target;
    }

    Gun_TargetInfo(weapon);
}

void Gun_ChangeTarget(WEAPON_INFO *const weapon)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->target = nullptr;
    bool found_new_target = false;

    for (int new_target = 0; new_target < LOT_SLOT_COUNT; new_target++) {
        if (!m_TargetList[new_target]) {
            break;
        }

        for (int last_target = 0; last_target < LOT_SLOT_COUNT; last_target++) {
            if (!m_LastTargetList[last_target]) {
                found_new_target = true;
                break;
            }

            if (m_LastTargetList[last_target] == m_TargetList[new_target]) {
                break;
            }
        }

        if (found_new_target) {
            lara->target = m_TargetList[new_target];
            break;
        }
    }

    if (lara->target != m_LastTargetList[0]) {
        for (int last_target = LOT_SLOT_COUNT - 1; last_target > 0;
             last_target--) {
            m_LastTargetList[last_target] = m_LastTargetList[last_target - 1];
        }
        m_LastTargetList[0] = lara->target;
    }

    Gun_TargetInfo(weapon);
}

int32_t Gun_FireWeapon(
    const LARA_GUN_TYPE weapon_type, ITEM *const target, const ITEM *const src,
    const int16_t *const angles)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    WEAPON_INFO *const weapon = &g_Weapons[weapon_type];

    AMMO_INFO *ammo;
    switch (weapon_type) {
    case LGT_MAGNUMS:
        ammo = &lara->magnum_ammo;
        if (Game_IsBonusFlagSet(GBF_NGPLUS)) {
            ammo->ammo = 1000;
        }
        break;

    case LGT_UZIS:
        ammo = &lara->uzi_ammo;
        if (Game_IsBonusFlagSet(GBF_NGPLUS)) {
            ammo->ammo = 1000;
        }
        break;

    case LGT_SHOTGUN:
        ammo = &lara->shotgun_ammo;
        if (Game_IsBonusFlagSet(GBF_NGPLUS)) {
            ammo->ammo = 1000;
        }
        break;

    default:
        ammo = &lara->pistol_ammo;
        ammo->ammo = 1000;
        break;
    }

    if (ammo->ammo <= 0) {
        ammo->ammo = 0;
        Sound_Effect(SFX_LARA_EMPTY, &src->pos, SPM_NORMAL);
        if (Inv_RequestItem(O_PISTOL_ITEM)) {
            lara->request_gun_type = LGT_PISTOLS;
        } else {
            lara->gun_status = LGS_UNDRAW;
        }
        return 0;
    }

    ammo->ammo--;
    Stats_AddAmmoUsed();

    const XYZ_32 view_pos = {
        .x = src->pos.x,
        .y = src->pos.y - weapon->gun_height,
        .z = src->pos.z,
    };
    const XYZ_16 view_rot = {
        .x = angles[1]
            + (weapon->shot_accuracy * (Random_GetControl() - DEG_90))
                / DEG_360,
        .y = angles[0]
            + (weapon->shot_accuracy * (Random_GetControl() - DEG_90))
                / DEG_360,
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
        if (ABS(sptr->pos.x) < r && ABS(sptr->pos.y) < r && sptr->pos.z > r
            && (sptr->pos.x * sptr->pos.x) + (sptr->pos.y * sptr->pos.y)
                <= (r * r)
            && (sptr->pos.z - r < bestdist)) {
            bestdist = sptr->pos.z - r;
            best = i;
        }
    }

    GAME_VECTOR vsrc;
    vsrc.room_num = src->room_num;
    vsrc.pos = view_pos;

    GAME_VECTOR vdest;
    if (best >= 0) {
        Stats_AddAmmoHits();
        vdest.x = vsrc.x + ((bestdist * g_MatrixPtr->_20) >> W2V_SHIFT);
        vdest.y = vsrc.y + ((bestdist * g_MatrixPtr->_21) >> W2V_SHIFT);
        vdest.z = vsrc.z + ((bestdist * g_MatrixPtr->_22) >> W2V_SHIFT);
        Gun_HitTarget(
            target, &vdest,
            weapon->damage * (Game_IsBonusFlagSet(GBF_JAPANESE) ? 2 : 1));
        return 1;
    }

    vdest.x = vsrc.x + g_MatrixPtr->_20;
    vdest.y = vsrc.y + g_MatrixPtr->_21;
    vdest.z = vsrc.z + g_MatrixPtr->_22;
    LOS_Check(&vsrc, &vdest);
    Spawn_Ricochet(&vdest);
    return -1;
}

void Gun_HitTarget(ITEM *item, GAME_VECTOR *hitpos, int16_t damage)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (item->hit_points > 0 && item->hit_points <= damage) {
        Stats_AddKill();
        if (g_Config.gameplay.target_mode == TLM_SEMI) {
            lara->target = nullptr;
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
