#include "game/gun/gun_misc.h"

#include "game/los.h"
#include "game/objects/general/window.h"
#include "game/objects/vars.h"
#include "game/spawn.h"
#include "game/stats.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/game/collision.h>
#include <libtrx/game/game.h>
#include <libtrx/game/gun.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/math.h>
#include <libtrx/game/matrix.h>
#include <libtrx/game/output.h>
#include <libtrx/game/random.h>
#include <libtrx/utils.h>

#define M_NEAR_ANGLE (DEG_1 * 15) // = 2730
#define M_ALLY_FRIENDLY_FIRE_THRESHOLD 10

static ITEM *m_TargetList[LOT_SLOT_COUNT] = {};
static ITEM *m_LastTargetList[LOT_SLOT_COUNT] = {};

static void M_SmashItem(int16_t item_num);

static void M_SmashItem(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    switch (item->object_id) {
    case O_WINDOW_1:
        Window_Smash(item_num);
        break;

    case O_BELL:
        if (item->status != IS_ACTIVE) {
            item->status = IS_ACTIVE;
            Item_AddActive(item_num);
        }
        break;

    default:
        break;
    }
}

void Gun_GetNewTarget(const WEAPON_INFO *const weapon)
{
    const ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara = Lara_GetLaraInfo();

    // Preserve OG targeting behavior.
    if (g_Config.gameplay.target_mode == TLM_FULL
        && !g_Config.gameplay.enable_target_change && !g_Input.action) {
        lara->target = nullptr;
    }

    const GAME_VECTOR start = {
        .x = lara_item->pos.x,
        .y = lara_item->pos.y - 650,
        .z = lara_item->pos.z,
        .room_num = lara_item->room_num,
    };

    ITEM *best_target = nullptr;
    int16_t best_y_rot = INT16_MAX;
    int16_t num_targets = 0;
    int32_t best_dist = INT32_MAX;

    const int32_t max_dist = weapon->target_dist;
    for (int32_t i = 0; i < LOT_SLOT_COUNT; i++) {
        const CREATURE *const creature = LOT_GetBaddieSlot(i);
        if (creature->item_num == NO_ITEM) {
            continue;
        }

        ITEM *const item = Item_Get(creature->item_num);
        if (item->hit_points <= 0) {
            continue;
        }

        if (!Creature_IsTargetable(item)) {
            continue;
        }

        const int32_t dx = item->pos.x - start.x;
        const int32_t dy = item->pos.y - start.y;
        const int32_t dz = item->pos.z - start.z;
        if (ABS(dx) > max_dist || ABS(dy) > max_dist || ABS(dz) > max_dist) {
            continue;
        }

        const int32_t dist = SQUARE(dx) + SQUARE(dy) + SQUARE(dz);
        if (dist >= SQUARE(max_dist)) {
            continue;
        }

        GAME_VECTOR target;
        Gun_FindTargetPoint(item, &target);
        if (!LOS_Check(&start, &target)) {
            continue;
        }

        int16_t angles[2];
        Math_GetVectorAngles(
            target.x - start.x, target.y - start.y, target.z - start.z, angles);
        angles[0] -= lara->torso_rot.y + lara_item->rot.y;
        angles[1] -= lara->torso_rot.x + lara_item->rot.x;

        if (angles[0] >= weapon->lock_angles[0]
            && angles[0] <= weapon->lock_angles[1]
            && angles[1] >= weapon->lock_angles[2]
            && angles[1] <= weapon->lock_angles[3]) {
            m_TargetList[num_targets] = item;
            num_targets++;
            const int16_t y_rot = ABS(angles[0]);
            if (y_rot < best_y_rot + M_NEAR_ANGLE && dist < best_dist) {
                best_dist = dist;
                best_y_rot = y_rot;
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
        for (int32_t slot = 0; slot < LOT_SLOT_COUNT; slot++) {
            if (m_TargetList[slot] == nullptr) {
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
        for (int32_t slot = LOT_SLOT_COUNT - 1; slot > 0; slot--) {
            m_LastTargetList[slot] = m_LastTargetList[slot - 1];
        }
        m_LastTargetList[0] = lara->target;
    }

    Gun_TargetInfo(weapon);
}

void Gun_ChangeTarget(const WEAPON_INFO *const weapon)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->target = nullptr;
    bool found_new_target = false;

    for (int32_t new_target = 0; new_target < LOT_SLOT_COUNT; new_target++) {
        if (!m_TargetList[new_target]) {
            break;
        }

        for (int32_t last_target = 0; last_target < LOT_SLOT_COUNT;
             last_target++) {
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
        for (int32_t last_target = LOT_SLOT_COUNT - 1; last_target > 0;
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
    const WEAPON_INFO *const weapon = &g_Weapons[weapon_type];
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    AMMO_INFO *const ammo = Gun_GetAmmoInfo(weapon_type);
    ASSERT(ammo != nullptr);

    if (ammo == &lara->pistol_ammo || Game_IsBonusFlagSet(GBF_NGPLUS)) {
        ammo->ammo = 1000;
    }

    if (ammo->ammo <= 0) {
        ammo->ammo = 0;
        return 0;
    }

    ammo->ammo--;

    const XYZ_32 view_pos = {
        .x = src->pos.x,
        .y = src->pos.y - weapon->gun_height,
        .z = src->pos.z,
    };
    const XYZ_16 view_rot = {
        .x = angles[1]
            + weapon->shot_accuracy * (Random_GetControl() - DEG_90) / DEG_360,
        .y = angles[0]
            + weapon->shot_accuracy * (Random_GetControl() - DEG_90) / DEG_360,
        .z = 0,
    };
    Matrix_GenerateW2V(&view_pos, &view_rot);

    SPHERE spheres[33];
    int32_t sphere_count = Collide_GetSpheres(target, spheres, false);
    int32_t best_sphere = -1;
    int32_t best_dist = 0x7FFFFFFF;

    for (int32_t i = 0; i < sphere_count; i++) {
        const SPHERE *const sphere = &spheres[i];
        const int32_t r = sphere->r;
        if (ABS(sphere->pos.x) < r && ABS(sphere->pos.y) < r
            && sphere->pos.z > r
            && SQUARE(sphere->pos.x) + SQUARE(sphere->pos.y) <= SQUARE(r)) {
            const int32_t dist = sphere->pos.z - r;
            if (dist < best_dist) {
                best_dist = dist;
                best_sphere = i;
            }
        }
    }

    Stats_AddAmmoUsed();

    GAME_VECTOR start;
    start.pos.x = view_pos.x;
    start.pos.y = view_pos.y;
    start.pos.z = view_pos.z;
    start.room_num = src->room_num;

    if (best_sphere < 0) {
        const int32_t dist = weapon->target_dist;
        GAME_VECTOR hit_pos;
        hit_pos.x = view_pos.x + ((dist * g_MatrixPtr->_20) >> W2V_SHIFT);
        hit_pos.y = view_pos.y + ((dist * g_MatrixPtr->_21) >> W2V_SHIFT);
        hit_pos.z = view_pos.z + ((dist * g_MatrixPtr->_22) >> W2V_SHIFT);
        hit_pos.room_num = src->room_num;
        Room_GetSector(hit_pos.x, hit_pos.y, hit_pos.z, &hit_pos.room_num);
        const bool object_on_los = LOS_Check(&start, &hit_pos);
        if (Gun_SmashItems(start.pos, hit_pos.pos, &hit_pos.pos)
            == PROJECTILE_HIT_STOP) {
            Room_GetSector(hit_pos.x, hit_pos.y, hit_pos.z, &hit_pos.room_num);
        }
        if (!object_on_los) {
            Spawn_Ricochet(&hit_pos);
        }
        return -1;
    } else {
        Stats_AddAmmoHits();
        GAME_VECTOR hit_pos;
        hit_pos.pos.x =
            view_pos.x + ((best_dist * g_MatrixPtr->_20) >> W2V_SHIFT);
        hit_pos.pos.y =
            view_pos.y + ((best_dist * g_MatrixPtr->_21) >> W2V_SHIFT);
        hit_pos.pos.z =
            view_pos.z + ((best_dist * g_MatrixPtr->_22) >> W2V_SHIFT);
        hit_pos.room_num =
            Room_GetIndexFromPos(hit_pos.pos.x, hit_pos.pos.y, hit_pos.pos.z);
        Gun_SmashItems(start.pos, hit_pos.pos, nullptr);
        Gun_HitTarget(
            target, &hit_pos,
            weapon->damage * (Game_IsBonusFlagSet(GBF_JAPANESE) ? 2 : 1));
        return 1;
    }
}

PROJECTILE_HIT Gun_SmashItems(
    const XYZ_32 start, const XYZ_32 target, XYZ_32 *const out_hit_pos)
{
    int32_t hits = 0;
    int16_t last_item_num = NO_ITEM;
    while (true) {
        const int16_t item_num = LOS_CheckSmashable(start, target, out_hit_pos);
        if (item_num == NO_ITEM || item_num == last_item_num) {
            break;
        }
        last_item_num = item_num;
        M_SmashItem(item_num);
        hits++;

        const ITEM *const item = Item_Get(item_num);
        if (Object_IsType(item->object_id, g_SmashableObjects)) {
            return PROJECTILE_HIT_STOP;
        }
    }
    return hits > 0 ? PROJECTILE_HIT_SHATTER : PROJECTILE_HIT_NONE;
}

void Gun_HitTarget(
    ITEM *const item, const GAME_VECTOR *const hit_pos, const int32_t damage)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (item->hit_points > 0 && item->hit_points <= damage
        && item->object_id != O_DRAGON_FRONT) {
        Stats_AddKill();
        if (g_Config.gameplay.target_mode == TLM_SEMI) {
            lara->target = nullptr;
        }
    }
    Item_TakeDamage(item, damage, true);

    if (hit_pos != nullptr) {
        Spawn_Blood(
            hit_pos->pos.x, hit_pos->pos.y, hit_pos->pos.z, item->speed,
            item->rot.y, item->room_num);
    }

    if (!Creature_AreAlliesHostile() && Creature_IsAlly(item)) {
        CREATURE *const creature = item->data;
        creature->flags += damage;
        if ((creature->flags & 0xFFF) > M_ALLY_FRIENDLY_FIRE_THRESHOLD
            || creature->mood == MOOD_BORED) {
            Creature_SetAlliesHostile(true);
        }
    }
}
