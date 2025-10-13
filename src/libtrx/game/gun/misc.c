#include "game/gun/misc.h"

#include "config.h"
#include "game/const.h"
#include "game/gun/common.h"
#include "game/gun/pistols.h"
#include "game/gun/rifle.h"
#include "game/inventory.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/los.h"
#include "game/math.h"
#include "game/matrix.h"
#include "game/output.h"
#include "game/pathing.h"
#include "game/random.h"
#include "game/sound.h"
#include "game/spawn.h"
#include "game/stats.h"

#if TR_VERSION >= 2
    #define M_ALLY_FRIENDLY_FIRE_THRESHOLD 10
    #define M_NEAR_ANGLE (DEG_1 * 15) // = 2730
#endif

static ITEM *m_TargetList[LOT_SLOT_COUNT] = {};
static ITEM *m_LastTargetList[LOT_SLOT_COUNT] = {};

#if TR_VERSION >= 2
// TODO: meh
extern void Window_Smash(int16_t item_num);
#endif

static void M_SmashItem(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    switch (item->object_id) {
#if TR_VERSION >= 2
    case O_WINDOW_1:
        Window_Smash(item_num);
        break;
#endif

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

    if (!LOS_Check(&start, &target)) {
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
        case LGT_UZIS:
            Gun_Pistols_DrawMeshes(lara->gun_type);
            break;

        case LGT_SHOTGUN:
#if TR_VERSION >= 2
        case LGT_M16:
        case LGT_GRENADE:
        case LGT_HARPOON:
#endif
            Gun_Rifle_DrawMeshes(lara->gun_type);
            break;

#if TR_VERSION >= 2
        case LGT_FLARE:
            Lara_Flare_DrawMeshes();
            break;
#endif

        default:
            break;
        }
    }
}

void Gun_DrawFlash(LARA_GUN_TYPE weapon_type, CLIP clip)
{
    int16_t shade;
    int32_t len;
    int32_t off;

    switch (weapon_type) {
    case LGT_MAGNUMS:
        shade = SHADE_NEUTRAL;
        len = TR_VERSION == 1 ? 155 : 215;
        off = TR_VERSION == 1 ? 55 : 65;
        break;

    case LGT_UZIS:
        shade = 10 * 256;
        len = TR_VERSION == 1 ? 180 : 200;
        off = TR_VERSION == 1 ? 55 : 50;
        break;

    case LGT_SHOTGUN:
#if TR_VERSION == 1
        if (!g_Config.visuals.enable_shotgun_flash) {
            return;
        }
        shade = 10 * 256;
        len = 285;
        off = 0;
        break;
#else
        return;
#endif

#if TR_VERSION >= 2
    case LGT_M16:
        shade = 10 * 256;
        len = 400;
        off = 99;
        Matrix_TranslateRel(0, len, off);
        Matrix_RotX(-85 * DEG_1);
        Matrix_RotZ(((2 * Random_GetDraw()) & 0x4000) + 0x2000);
        Output_CalculateStaticLight(shade);
        const OBJECT *const obj = Object_Get(O_M16_FLASH);
        if (obj->loaded) {
            Object_DrawMesh(obj->mesh_idx, clip, false);
        }
        return;

    case LGT_FLARE:
        Matrix_TranslateRel(11, 32, 80);
        Matrix_RotX(-DEG_90);
        Matrix_RotY(2 * Random_GetDraw());
        Output_CalculateStaticLight(2048);
        Object_DrawMesh(Object_Get(O_FLARE_FIRE)->mesh_idx, clip, false);
        return;

#endif

    default:
        shade = SHADE_LOW;
        len = TR_VERSION == 1 ? 155 : 185;
        off = TR_VERSION == 1 ? 55 : 40;
        break;
    }

    Matrix_TranslateRel(0, len, off);
    Matrix_RotX(-DEG_90);
    Matrix_RotZ(2 * Random_GetDraw());
    Output_CalculateStaticLight(shade);
    const OBJECT *const obj = Object_Get(O_GUN_FLASH);
    if (obj->loaded) {
        Object_DrawMesh(obj->mesh_idx, clip, false);
    }
}

void Gun_UpdateLaraMeshes(const OBJECT_ID obj_id)
{
    const bool lara_has_rifle = Inv_RequestItem(O_SHOTGUN_ITEM)
        || Inv_RequestItem(O_HARPOON_ITEM) || Inv_RequestItem(O_M16_ITEM)
        || Inv_RequestItem(O_GRENADE_ITEM);
    const bool lara_has_pistols = Inv_RequestItem(O_PISTOL_ITEM)
        || Inv_RequestItem(O_MAGNUM_ITEM) || Inv_RequestItem(O_UZI_ITEM);

    LARA_GUN_TYPE back_gun_type = LGT_UNARMED;
    LARA_GUN_TYPE holsters_gun_type = LGT_UNARMED;

    if (!lara_has_rifle && obj_id == O_SHOTGUN_ITEM) {
        back_gun_type = LGT_SHOTGUN;
#if TR_VERSION >= 2
    } else if (!lara_has_rifle && obj_id == O_HARPOON_ITEM) {
        back_gun_type = LGT_HARPOON;
    } else if (!lara_has_rifle && obj_id == O_M16_ITEM) {
        back_gun_type = LGT_M16;
    } else if (!lara_has_rifle && obj_id == O_GRENADE_ITEM) {
        back_gun_type = LGT_GRENADE;
#endif
    } else if (!lara_has_pistols && obj_id == O_PISTOL_ITEM) {
        holsters_gun_type = LGT_PISTOLS;
    } else if (!lara_has_pistols && obj_id == O_MAGNUM_ITEM) {
        holsters_gun_type = LGT_MAGNUMS;
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
    ITEM *const item, const GAME_VECTOR *const hit_pos, const int32_t damage)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (item->hit_points > 0 && item->hit_points <= damage) {
        const bool skip_stats = item->object_id == O_DRAGON_FRONT;

        if (!skip_stats) {
            Stats_AddKill();
        }
        if (g_Config.gameplay.target_mode == TLM_SEMI) {
            lara->target = nullptr;
        }
    }
    Item_TakeDamage(item, damage, true);

    if (hit_pos != nullptr) {
        const bool make_ricochet = g_Config.visuals.fix_texture_issues
            && item->object_id == O_SCION_ITEM_3;

        if (make_ricochet) {
            const GAME_VECTOR pos = {
                .pos = hit_pos->pos,
                .room_num = item->room_num,
            };
            Spawn_Ricochet(&pos);
        } else {
            Spawn_Blood(
                hit_pos->x, hit_pos->y, hit_pos->z, item->speed, item->rot.y,
                item->room_num);
        }
    }

#if TR_VERSION == 1
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
#else
    if (!Creature_AreAlliesHostile() && Creature_IsAlly(item)) {
        CREATURE *const creature = item->data;
        creature->flags += damage;
        if ((creature->flags & 0xFFF) > M_ALLY_FRIENDLY_FIRE_THRESHOLD
            || creature->mood == MOOD_BORED) {
            Creature_SetAlliesHostile(true);
        }
    }
#endif
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

#if TR_VERSION == 1
    // This way of iteration properly lets Lara target mummy in Qualopec.
    int16_t item_num = Item_GetNextActive();
    while (item_num != NO_ITEM) {
        ITEM *const item = Item_Get(item_num);
        item_num = item->next_active;
#else
    // This way of iteration properly lets Lara damage black skidoo riders.
    for (int32_t i = 0; i < LOT_SLOT_COUNT; i++) {
        const CREATURE *const creature = LOT_GetBaddieSlot(i);
        if (creature->item_num == NO_ITEM) {
            continue;
        }

        ITEM *const item = Item_Get(creature->item_num);
#endif
        if (item->hit_points <= 0) {
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
#if TR_VERSION == 1
            if (y_rot < best_y_rot) {
#else
            if (y_rot < best_y_rot + M_NEAR_ANGLE && dist < best_dist) {
#endif
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
