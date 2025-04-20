#include "game/gun/gun_misc.h"

#include "game/gun/gun.h"
#include "game/items.h"
#include "game/los.h"
#include "game/objects/general/window.h"
#include "game/output.h"
#include "game/random.h"
#include "game/room.h"
#include "game/spawn.h"
#include "game/stats.h"
#include "global/vars.h"

#include <libtrx/debug.h>
#include <libtrx/game/collision.h>
#include <libtrx/game/game.h>
#include <libtrx/game/math.h>
#include <libtrx/game/matrix.h>
#include <libtrx/utils.h>

#define NEAR_ANGLE (DEG_1 * 15) // = 2730

static LARA_STATE m_HoldStates[] = {
    LS_WALK,   LS_STOP,      LS_POSE,       LS_TURN_RIGHT,  LS_TURN_LEFT,
    LS_BACK,   LS_FAST_TURN, LS_STEP_LEFT,  LS_STEP_RIGHT,  LS_WADE,
    LS_PICKUP, LS_SWITCH_ON, LS_SWITCH_OFF, (LARA_STATE)-1,
};

int32_t Gun_CheckForHoldingState(const LARA_STATE state)
{
    if (g_Lara.extra_anim) {
        return false;
    }

    const LARA_STATE *hold_state = m_HoldStates;
    while (*hold_state != (LARA_STATE)-1) {
        if (*hold_state == state) {
            return true;
        }
        hold_state++;
    }
    return false;
}

void Gun_TargetInfo(const WEAPON_INFO *const winfo)
{
    if (!g_Lara.target) {
        g_Lara.left_arm.lock = 0;
        g_Lara.right_arm.lock = 0;
        g_Lara.target_angles[0] = 0;
        g_Lara.target_angles[1] = 0;
        return;
    }

    GAME_VECTOR start;
    start.pos.x = g_LaraItem->pos.x;
    start.pos.y = g_LaraItem->pos.y - 650;
    start.pos.z = g_LaraItem->pos.z;
    start.room_num = g_LaraItem->room_num;

    GAME_VECTOR target;
    Gun_FindTargetPoint(g_Lara.target, &target);

    int16_t angles[2];
    // clang-format off
    Math_GetVectorAngles(
        target.pos.x - start.pos.x,
        target.pos.y - start.pos.y,
        target.pos.z - start.pos.z,
        angles);
    // clang-format on

    angles[0] -= g_LaraItem->rot.y;
    angles[1] -= g_LaraItem->rot.x;

    if (!LOS_Check(&start, &target)) {
        g_Lara.right_arm.lock = 0;
        g_Lara.left_arm.lock = 0;
    } else if (
        angles[0] >= winfo->lock_angles[0] && angles[0] <= winfo->lock_angles[1]
        && angles[1] >= winfo->lock_angles[2]
        && angles[1] <= winfo->lock_angles[3]) {
        g_Lara.right_arm.lock = 1;
        g_Lara.left_arm.lock = 1;
    } else {
        if (g_Lara.left_arm.lock
            && (angles[0] < winfo->left_angles[0]
                || angles[0] > winfo->left_angles[1]
                || angles[1] < winfo->left_angles[2]
                || angles[1] > winfo->left_angles[3])) {
            g_Lara.left_arm.lock = 0;
        }
        if (g_Lara.right_arm.lock
            && (angles[0] < winfo->right_angles[0]
                || angles[0] > winfo->right_angles[1]
                || angles[1] < winfo->right_angles[2]
                || angles[1] > winfo->right_angles[3])) {
            g_Lara.right_arm.lock = 0;
        }
    }

    g_Lara.target_angles[0] = angles[0];
    g_Lara.target_angles[1] = angles[1];
}

void Gun_GetNewTarget(const WEAPON_INFO *const winfo)
{
    GAME_VECTOR start;
    start.pos.x = g_LaraItem->pos.x;
    start.pos.y = g_LaraItem->pos.y - 650;
    start.pos.z = g_LaraItem->pos.z;
    start.room_num = g_LaraItem->room_num;

    int16_t best_y_rot = 0x7FFF;
    int32_t best_dist = 0x7FFFFFFF;
    ITEM *best_target = nullptr;

    const int16_t max_dist = winfo->target_dist;
    for (int32_t i = 0; i < LOT_SLOT_COUNT; i++) {
        const int16_t item_num = g_BaddieSlots[i].item_num;
        if (item_num == NO_ITEM || item_num == g_Lara.item_num) {
            continue;
        }

        ITEM *const item = Item_Get(item_num);
        if (item->hit_points <= 0) {
            continue;
        }

        const int32_t dx = item->pos.x - start.pos.x;
        const int32_t dy = item->pos.y - start.pos.y;
        const int32_t dz = item->pos.z - start.pos.z;
        if (ABS(dx) > max_dist || ABS(dy) > max_dist || ABS(dz) > max_dist) {
            continue;
        }

        const int32_t dist = SQUARE(dz) + SQUARE(dy) + SQUARE(dx);
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
            target.pos.x - start.pos.x, target.pos.y - start.pos.y,
            target.pos.z - start.pos.z, angles);
        angles[0] -= g_Lara.torso_rot.y + g_LaraItem->rot.y;
        angles[1] -= g_Lara.torso_rot.x + g_LaraItem->rot.x;

        if (angles[0] >= winfo->lock_angles[0]
            && angles[0] <= winfo->lock_angles[1]
            && angles[1] >= winfo->lock_angles[2]
            && angles[1] <= winfo->lock_angles[3]) {
            const int16_t y_rot = ABS(angles[0]);
            if (y_rot < best_y_rot + NEAR_ANGLE && dist < best_dist) {
                best_dist = dist;
                best_y_rot = y_rot;
                best_target = item;
            }
        }
    }

    g_Lara.target = best_target;
    Gun_TargetInfo(winfo);
}

void Gun_AimWeapon(const WEAPON_INFO *const winfo, LARA_ARM *const arm)
{
    const int16_t speed = winfo->aim_speed;

    int16_t dest_y = 0;
    int16_t dest_x = 0;
    if (arm->lock) {
        dest_y = g_Lara.target_angles[0];
        dest_x = g_Lara.target_angles[1];
    }

    if (arm->rot.y >= dest_y - speed && arm->rot.y <= dest_y + speed) {
        arm->rot.y = dest_y;
    } else if (arm->rot.y < dest_y) {
        arm->rot.y += speed;
    } else {
        arm->rot.y -= speed;
    }

    if (arm->rot.x >= dest_x - speed && arm->rot.x <= dest_x + speed) {
        arm->rot.x = dest_x;
    } else if (arm->rot.x < dest_x) {
        arm->rot.x += speed;
    } else {
        arm->rot.x -= speed;
    }

    arm->rot.z = 0;
}

int32_t Gun_FireWeapon(
    const LARA_GUN_TYPE weapon_type, ITEM *const target, const ITEM *const src,
    const int16_t *const angles)
{
    const WEAPON_INFO *const winfo = &g_Weapons[weapon_type];
    AMMO_INFO *const ammo = Gun_GetAmmoInfo(weapon_type);
    ASSERT(ammo != nullptr);

    if (ammo == &g_Lara.pistol_ammo || Game_IsBonusFlagSet(GBF_NGPLUS)) {
        ammo->ammo = 1000;
    }

    if (ammo->ammo <= 0) {
        ammo->ammo = 0;
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
            + winfo->shot_accuracy * (Random_GetControl() - DEG_90) / DEG_360,
        .y = angles[0]
            + winfo->shot_accuracy * (Random_GetControl() - DEG_90) / DEG_360,
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
        const int32_t dist = winfo->target_dist;
        GAME_VECTOR hit_pos;
        hit_pos.pos.x = view_pos.x + ((dist * g_MatrixPtr->_20) >> W2V_SHIFT);
        hit_pos.pos.y = view_pos.y + ((dist * g_MatrixPtr->_21) >> W2V_SHIFT);
        hit_pos.pos.z = view_pos.z + ((dist * g_MatrixPtr->_22) >> W2V_SHIFT);
        hit_pos.room_num =
            Room_GetIndexFromPos(hit_pos.pos.x, hit_pos.pos.y, hit_pos.pos.z);
        const bool object_on_los = LOS_Check(&start, &hit_pos);
        const int16_t item_to_smash = LOS_CheckSmashable(&start, &hit_pos);
        if (item_to_smash == NO_ITEM) {
            if (!object_on_los) {
                Spawn_Ricochet(&hit_pos);
            }
            return -1;
        } else {
            Gun_SmashItem(item_to_smash, weapon_type);
            return -1;
        }
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
        const int16_t item_to_smash = LOS_CheckSmashable(&start, &hit_pos);
        if (item_to_smash != NO_ITEM) {
            Gun_SmashItem(item_to_smash, weapon_type);
        }
        Gun_HitTarget(
            target, &hit_pos,
            winfo->damage * (Game_IsBonusFlagSet(GBF_JAPANESE) ? 2 : 1));
        return 1;
    }
}

void Gun_FindTargetPoint(const ITEM *const item, GAME_VECTOR *const target)
{
    const BOUNDS_16 *const bounds = &Item_GetBestFrame(item)->bounds;
    const int32_t x = bounds->min.x + (bounds->max.x - bounds->min.x) / 2;
    const int32_t y = bounds->min.y + (bounds->max.y - bounds->min.y) / 3;
    const int32_t z = bounds->min.z + (bounds->max.z - bounds->min.z) / 2;
    const int32_t cy = Math_Cos(item->rot.y);
    const int32_t sy = Math_Sin(item->rot.y);
    target->pos.x = item->pos.x + ((cy * x + sy * z) >> W2V_SHIFT);
    target->pos.y = item->pos.y + y;
    target->pos.z = item->pos.z + ((cy * z - sy * x) >> W2V_SHIFT);
    target->room_num = item->room_num;
}

void Gun_HitTarget(
    ITEM *const item, const GAME_VECTOR *const hit_pos, const int32_t damage)
{
    if (item->hit_points > 0 && item->hit_points <= damage
        && item->object_id != O_DRAGON_FRONT) {
        Stats_AddKill();
    }
    Item_TakeDamage(item, damage, true);

    if (hit_pos != nullptr) {
        Spawn_Blood(
            hit_pos->pos.x, hit_pos->pos.y, hit_pos->pos.z, item->speed,
            item->rot.y, item->room_num);
    }

    if (!Creature_AreAlliesHostile()
        && (item->object_id == O_MONK_1 || item->object_id == O_MONK_2)) {
        CREATURE *const creature = item->data;
        creature->flags += damage;
        if ((creature->flags & 0xFFF) > MONK_FRIENDLY_FIRE_THRESHOLD
            || creature->mood == MOOD_BORED) {
            Creature_SetAlliesHostile(true);
        }
    }
}

void Gun_SmashItem(const int16_t item_num, const LARA_GUN_TYPE weapon_type)
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

void Gun_DrawFlash(const LARA_GUN_TYPE weapon_type, const int32_t clip)
{
    int16_t shade;
    int32_t y;
    int32_t z;

    switch (weapon_type) {
    case LGT_MAGNUMS:
        shade = SHADE_NEUTRAL;
        y = 215;
        z = 65;
        break;

    case LGT_UZIS:
        shade = 2560;
        y = 200;
        z = 50;
        break;

    case LGT_SHOTGUN:
        return;

    case LGT_M16:
        Matrix_TranslateRel(0, 400, 99);
        Matrix_RotX(-85 * DEG_1);
        Matrix_RotZ(((2 * Random_GetDraw()) & 0x4000) + 0x2000);
        Output_CalculateStaticLight(2560);
        Object_DrawMesh(Object_Get(O_M16_FLASH)->mesh_idx, clip, false);
        return;

    case LGT_FLARE:
        Matrix_TranslateRel(11, 32, 80);
        Matrix_RotX(-DEG_90);
        Matrix_RotY(2 * Random_GetDraw());
        Output_CalculateStaticLight(2048);
        Object_DrawMesh(Object_Get(O_FLARE_FIRE)->mesh_idx, clip, false);
        return;

    default:
        shade = SHADE_LOW;
        y = 185;
        z = 40;
        break;
    }

    Matrix_TranslateRel(0, y, z);
    Matrix_RotX(-DEG_90);
    Matrix_RotZ(2 * Random_GetDraw());
    Output_CalculateStaticLight(shade);
    Object_DrawMesh(Object_Get(O_GUN_FLASH)->mesh_idx, clip, false);
    return;
}
