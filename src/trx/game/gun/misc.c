#include <trx/game/gun/misc.h>

#include <trx/config.h>
#include <trx/game/const.h>
#include <trx/game/creature.h>
#include <trx/game/gun/common.h>
#include <trx/game/gun/pistols.h>
#include <trx/game/gun/rifle.h>
#include <trx/game/gun/vars.h>
#include <trx/game/inventory.h>
#include <trx/game/items.h>
#include <trx/game/lara.h>
#include <trx/game/los.h>
#include <trx/game/math.h>
#include <trx/game/matrix.h>
#include <trx/game/output.h>
#include <trx/game/pathing.h>
#include <trx/game/random.h>
#include <trx/game/sound.h>
#include <trx/game/spawn.h>
#include <trx/game/stats.h>

#define M_NEAR_ANGLE (DEG_1 * 15) // = 2730

static ITEM *m_TargetList[LOT_SLOT_COUNT] = {};
static ITEM *m_LastTargetList[LOT_SLOT_COUNT] = {};

// TODO: meh
extern void Window_Smash(int16_t item_num);

static void M_DrawGunGlow(const XYZ_32 offset, const RGB_F color)
{
    if (g_TRVersion < 3) {
        return;
    }
    const OBJECT *const glow_obj = Object_Get(O_GLOW);
    if (!glow_obj->loaded) {
        return;
    }

    Matrix_Push();
    Matrix_TranslateRel32(offset);
    const XYZ_32 pos = {
        .x = (int32_t)(g_WMatrixPtr->_03 >> W2V_SHIFT),
        .y = (int32_t)(g_WMatrixPtr->_13 >> W2V_SHIFT),
        .z = (int32_t)(g_WMatrixPtr->_23 >> W2V_SHIFT),
    };
    Matrix_Pop();
    Output_DrawSprite(
        pos.x, pos.y, pos.z, glow_obj->mesh_idx, 0, color, DRAW_BLEND_ADD);
}

static void M_SmashItem(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    switch (item->object_id) {
    case O_WINDOW_1:
        Window_Smash(item_num);
        break;

    case O_BELL:
    case O_CARCASS:
        if (item->status != IS_ACTIVE) {
            item->status = IS_ACTIVE;
            Item_AddActive(item_num);
        }
        break;

    case O_SCION_ITEM_3:
        Gun_HitTarget(item, nullptr, nullptr, item->hit_points);
        break;

    default:
        break;
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

void Gun_AimWeapon(const WEAPON_INFO *const weapon, LARA_ARM *const arm)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    const int16_t speed = weapon->aim_speed;

    int16_t dest_x = 0;
    int16_t dest_y = 0;
    if (arm->lock) {
        dest_y = lara->target_angles[0];
        dest_x = lara->target_angles[1];
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

void Gun_TargetInfo(const WEAPON_INFO *const weapon)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    const ITEM *const lara_item = Lara_GetItem();
    if (lara->target == nullptr) {
        lara->left_arm.lock = 0;
        lara->right_arm.lock = 0;
        lara->target_angles[0] = 0;
        lara->target_angles[1] = 0;
        return;
    }

    GAME_VECTOR target;
    GAME_VECTOR start = {
        .pos = {
            .x = lara_item->pos.x,
            .y = lara_item->pos.y - 650,
            .z = lara_item->pos.z,
        },
        .room_num = lara_item->room_num,
    };
    Gun_FindTargetPoint(lara->target, &target);

    int16_t angles[2];
    // clang-format off
    Math_GetVectorAngles(
        target.pos.x - start.pos.x,
        target.pos.y - start.pos.y,
        target.pos.z - start.pos.z,
        angles);
    // clang-format on

    angles[0] -= lara_item->rot.y;
    angles[1] -= lara_item->rot.x;

    if (!LOS_Check(&start, &target, true)) {
        lara->left_arm.lock = 0;
        lara->right_arm.lock = 0;
    } else if (
        angles[0] >= weapon->lock_angles[0]
        && angles[0] <= weapon->lock_angles[1]
        && angles[1] >= weapon->lock_angles[2]
        && angles[1] <= weapon->lock_angles[3]) {
        lara->left_arm.lock = 1;
        lara->right_arm.lock = 1;
    } else {
        if (lara->left_arm.lock
            && (angles[0] < weapon->left_angles[0]
                || angles[0] > weapon->left_angles[1]
                || angles[1] < weapon->left_angles[2]
                || angles[1] > weapon->left_angles[3])) {
            lara->left_arm.lock = 0;
        }
        if (lara->right_arm.lock
            && (angles[0] < weapon->right_angles[0]
                || angles[0] > weapon->right_angles[1]
                || angles[1] < weapon->right_angles[2]
                || angles[1] > weapon->right_angles[3])) {
            lara->right_arm.lock = 0;
        }
    }

    lara->target_angles[0] = angles[0];
    lara->target_angles[1] = angles[1];
}

void Gun_InitialiseNewWeapon(void)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->target = nullptr;

    lara->left_arm.flash_gun = 0;
    lara->left_arm.frame_num = LF_G_AIM_START;
    lara->left_arm.lock = 0;
    lara->left_arm.rot.x = 0;
    lara->left_arm.rot.y = 0;
    lara->left_arm.rot.z = 0;

    lara->right_arm.flash_gun = 0;
    lara->right_arm.frame_num = LF_G_AIM_START;
    lara->right_arm.lock = 0;
    lara->right_arm.rot.x = 0;
    lara->right_arm.rot.y = 0;
    lara->right_arm.rot.z = 0;

    const OBJECT_ID anim_type = Gun_GetLaraAnim(lara->gun_type);
    const OBJECT *const obj = Object_Get(anim_type);
    lara->left_arm.frame_base = obj->frame_base;
    lara->right_arm.frame_base = obj->frame_base;

    if (lara->gun_status != LGS_ARMLESS) {
        switch (lara->gun_type) {
        case LGT_PISTOLS:
        case LGT_MAGNUMS:
        case LGT_AUTOS:
        case LGT_DESERT_EAGLE:
        case LGT_UZIS:
            Gun_Pistols_DrawMeshes(lara->gun_type);
            break;

        case LGT_SHOTGUN:
        case LGT_M16:
        case LGT_MP5:
        case LGT_GRENADE:
        case LGT_ROCKET:
        case LGT_HARPOON:
            Gun_Rifle_DrawMeshes(lara->gun_type);
            break;

        case LGT_FLARE:
            Lara_Flare_DrawMeshes();
            break;

        default:
            break;
        }
    }
}

void Gun_DrawFlash(
    const LARA_GUN_TYPE weapon_type, const CLIP clip, const bool interpolated)
{
    if (weapon_type == LGT_SHOTGUN && !g_Config.visuals.enable_shotgun_flash) {
        return;
    }

    OBJECT_ID flash_object_id = O_GUN_FLASH;
    XYZ_16 rot = {};

    switch (weapon_type) {
    case LGT_M16:
    case LGT_MP5:
        rot.x = -85 * DEG_1;
        rot.z = ((2 * Random_GetDraw()) & 0x4000);
        if (weapon_type == LGT_M16) {
            rot.z += 0x2000;
        } else {
            rot.z += (Random_GetDraw() & 0xFFF) + 0x1800;
        }
        flash_object_id = O_M16_FLASH;
        break;

    case LGT_FLARE:
        rot.x = -DEG_90;
        rot.y = 2 * Random_GetDraw();
        flash_object_id = O_FLARE_FIRE;
        break;

    default:
        rot.x = -DEG_90;
        rot.z = 2 * Random_GetDraw();
        break;
    }

    const WEAPON_INFO weapon = g_Weapons[weapon_type];
    if (interpolated) {
        Matrix_TranslateRel32_I(weapon.flash_pos);
        Matrix_RotX_I(rot.x);
        Matrix_RotY_I(rot.y);
        Matrix_RotZ_I(rot.z);
    } else {
        Matrix_TranslateRel32(weapon.flash_pos);
        Matrix_RotX(rot.x);
        Matrix_RotY(rot.y);
        Matrix_RotZ(rot.z);
    }

    if (g_TRVersion < 3) {
        Output_CalculateStaticLight(weapon.flash_shade);
    } else {
        Output_CalculateStaticLightRGB_F(weapon.flash_color);
    }
    const OBJECT *const flash_obj = Object_Get(flash_object_id);
    if (flash_obj->loaded) {
        Object_DrawMesh(flash_obj->mesh_idx, clip, interpolated);
    }

    M_DrawGunGlow(weapon.glow_pos, weapon.glow_color);
}

void Gun_UpdateLaraMeshes(const OBJECT_ID obj_id)
{
    const bool lara_has_rifle = Inv_RequestItem(O_SHOTGUN_ITEM)
        || Inv_RequestItem(O_HARPOON_ITEM) || Inv_RequestItem(O_M16_ITEM)
        || Inv_RequestItem(O_MP5_ITEM) || Inv_RequestItem(O_GRENADE_GUN_ITEM)
        || Inv_RequestItem(O_ROCKET_GUN_ITEM);
    const bool lara_has_pistols = Inv_RequestItem(O_PISTOL_ITEM)
        || Inv_RequestItem(O_MAGNUM_ITEM) || Inv_RequestItem(O_AUTOS_ITEM)
        || Inv_RequestItem(O_DESERT_EAGLE_ITEM) || Inv_RequestItem(O_UZI_ITEM);

    LARA_GUN_TYPE back_gun_type = LGT_UNARMED;
    LARA_GUN_TYPE holsters_gun_type = LGT_UNARMED;

    if (!lara_has_rifle && obj_id == O_SHOTGUN_ITEM) {
        back_gun_type = LGT_SHOTGUN;
    } else if (!lara_has_rifle && obj_id == O_HARPOON_ITEM) {
        back_gun_type = LGT_HARPOON;
    } else if (!lara_has_rifle && obj_id == O_M16_ITEM) {
        back_gun_type = LGT_M16;
    } else if (!lara_has_rifle && obj_id == O_MP5_ITEM) {
        back_gun_type = LGT_MP5;
    } else if (!lara_has_rifle && obj_id == O_GRENADE_GUN_ITEM) {
        back_gun_type = LGT_GRENADE;
    } else if (!lara_has_rifle && obj_id == O_ROCKET_GUN_ITEM) {
        back_gun_type = LGT_ROCKET;
    } else if (!lara_has_pistols && obj_id == O_PISTOL_ITEM) {
        holsters_gun_type = LGT_PISTOLS;
    } else if (!lara_has_pistols && obj_id == O_MAGNUM_ITEM) {
        holsters_gun_type = LGT_MAGNUMS;
    } else if (!lara_has_pistols && obj_id == O_AUTOS_ITEM) {
        holsters_gun_type = LGT_AUTOS;
    } else if (!lara_has_pistols && obj_id == O_DESERT_EAGLE_ITEM) {
        holsters_gun_type = LGT_DESERT_EAGLE;
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
    ITEM *const item, const GAME_VECTOR *const start,
    const GAME_VECTOR *const hit_pos, int32_t damage)
{
    const bool make_ricochet = !Item_ShouldSpawnBlood(item);
    if (item->object_id == O_SHIVA && make_ricochet) {
        damage = 0;
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    if ((item->hit_points == DONT_TARGET && Creature_IsDestructible(item))
        || (item->hit_points > 0 && item->hit_points <= damage)) {
        const bool skip_stats = item->object_id == O_DRAGON_FRONT;
        if (!skip_stats) {
            Stats_AddKill();
        }
        if (g_Config.gameplay.target_mode == TLM_SEMI) {
            lara->target = nullptr;
        }
    }

    Item_TakeDamage(item, damage, true);
    if (item->creature_data != nullptr
        && Object_Get(item->object_id)->intelligent) {
        Creature_Hurt(item, damage);
    }

    if (hit_pos != nullptr) {
        if (make_ricochet) {
            const GAME_VECTOR pos = {
                .pos = hit_pos->pos,
                .room_num = item->room_num,
            };
            Spawn_RicochetRay(*start, pos);
        } else {
            Spawn_Blood(
                hit_pos->x, hit_pos->y, hit_pos->z, item->speed, item->rot.y,
                item->room_num);
        }
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
        if (!LOS_Check(&start, &target, true)) {
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
            if (g_TRVersion == 1) {
                if (y_rot < best_y_rot) {
                    best_dist = dist;
                    best_y_rot = y_rot;
                    best_target = item;
                }
            } else {
                if (y_rot < best_y_rot + M_NEAR_ANGLE && dist < best_dist) {
                    best_dist = dist;
                    best_y_rot = y_rot;
                    best_target = item;
                }
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
