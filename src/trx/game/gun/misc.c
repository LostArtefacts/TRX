#include <trx/game/gun/misc.h>

#include <trx/config.h>
#include <trx/core/math.h>
#include <trx/core/utils.h>
#include <trx/game/creature.h>
#include <trx/game/gun/common.h>
#include <trx/game/gun/flare.h>
#include <trx/game/gun/pistols.h>
#include <trx/game/gun/registry.h>
#include <trx/game/gun/rifle.h>
#include <trx/game/input.h>
#include <trx/game/inventory.h>
#include <trx/game/items.h>
#include <trx/game/lara.h>
#include <trx/game/los.h>
#include <trx/game/matrix.h>
#include <trx/game/output.h>
#include <trx/game/output/water.h>
#include <trx/game/pathing.h>
#include <trx/game/random.h>
#include <trx/game/sound.h>
#include <trx/game/spawn.h>

#define M_NEAR_ANGLE (DEG_1 * 15) // = 2730

typedef struct {
    const WEAPON_INFO *weapon;
    const GAME_VECTOR *start;
    const ITEM *lara_item;
    const LARA_INFO *lara;
    const ITEM *old_target;
    int32_t max_dist;
    ITEM *best_target;
    int16_t best_y_rot;
    int32_t best_dist;
    int16_t num_targets;
    int32_t old_target_dist;
    int16_t old_target_y_rot;
    bool old_target_in_list;
} M_TARGET_CONTEXT;

static ITEM *m_TargetList[LOT_SLOT_COUNT] = {};
static ITEM *m_LastTargetList[LOT_SLOT_COUNT] = {};
static ITEM *m_BestTarget = nullptr;
static int16_t m_TargetCount = 0;

static bool M_TargetListContains(const ITEM *const item, const int16_t count)
{
    for (int16_t i = 0; i < count; i++) {
        if (m_TargetList[i] == item) {
            return true;
        }
    }
    return false;
}

static void M_ConsiderTarget(M_TARGET_CONTEXT *const ctx, ITEM *const item)
{
    if (item == ctx->lara_item || !Item_IsTargetable(item)) {
        return;
    }

    const int32_t dx = item->pos.x - ctx->start->x;
    const int32_t dy = item->pos.y - ctx->start->y;
    const int32_t dz = item->pos.z - ctx->start->z;
    if (ABS(dx) > ctx->max_dist || ABS(dy) > ctx->max_dist
        || ABS(dz) > ctx->max_dist) {
        return;
    }

    const int32_t dist = SQUARE(dx) + SQUARE(dy) + SQUARE(dz);
    if (dist >= SQUARE(ctx->max_dist)) {
        return;
    }

    GAME_VECTOR target;
    Gun_FindTargetPoint(item, &target);
    if (!LOS_Check(ctx->start, &target, true)) {
        return;
    }

    int16_t angles[2];
    Math_GetVectorAngles(
        target.x - ctx->start->x, target.y - ctx->start->y,
        target.z - ctx->start->z, angles);
    angles[0] -= ctx->lara->torso_rot.y + ctx->lara_item->rot.y;
    angles[1] -= ctx->lara->torso_rot.x + ctx->lara_item->rot.x;

    if (angles[0] < ctx->weapon->lock.min_yaw
        || angles[0] > ctx->weapon->lock.max_yaw
        || angles[1] < ctx->weapon->lock.min_pitch
        || angles[1] > ctx->weapon->lock.max_pitch) {
        return;
    }

    if (ctx->num_targets < LOT_SLOT_COUNT) {
        m_TargetList[ctx->num_targets] = item;
        ctx->num_targets++;
    }

    const int16_t y_rot = ABS(angles[0]);
    if (item == ctx->old_target) {
        ctx->old_target_dist = dist;
        ctx->old_target_y_rot = y_rot;
        ctx->old_target_in_list = true;
    }

    if (g_TRVersion == 1) {
        if (y_rot < ctx->best_y_rot) {
            ctx->best_dist = dist;
            ctx->best_y_rot = y_rot;
            ctx->best_target = item;
        }
    } else {
        if (y_rot < ctx->best_y_rot + M_NEAR_ANGLE && dist < ctx->best_dist) {
            ctx->best_dist = dist;
            ctx->best_y_rot = y_rot;
            ctx->best_target = item;
        }
    }
}

static void M_DrawGunGlow(
    const WEAPON_INFO *const weapon, const bool interpolated)
{
    if (g_TRVersion < 3 && !g_Config.visuals.enable_gun_glow) {
        return;
    }
    if (weapon->glow.scale <= 0.0f) {
        return;
    }
    const OBJECT *const glow_obj = Object_Get(O_GLOW);
    if (!glow_obj->loaded) {
        return;
    }

    // The glow follows a mesh that may be drawn between two game frames, so
    // the sprite position has to be interpolated the same way.
    if (interpolated) {
        Matrix_Push_I();
        Matrix_TranslateRel32_I(weapon->glow.pos);
        Matrix_Interpolate();
    } else {
        Matrix_Push();
        Matrix_TranslateRel32(weapon->glow.pos);
    }
    const XYZ_32 pos = {
        .x = (int32_t)(g_WMatrixPtr->_03 >> W2V_SHIFT),
        .y = (int32_t)(g_WMatrixPtr->_13 >> W2V_SHIFT),
        .z = (int32_t)(g_WMatrixPtr->_23 >> W2V_SHIFT),
    };
    if (interpolated) {
        Matrix_Pop_I();
    } else {
        Matrix_Pop();
    }

    // The flare's glow pulses as its pyro burns; gunfire glows are steady.
    const int16_t shade =
        weapon->glow.flicker ? (Random_GetDraw() & 0xFFF) + SHADE_NEUTRAL : 0;
    Output_DrawSprite(
        pos.x, pos.y, pos.z, glow_obj->mesh_idx, shade,
        Color_RGBToRGBA(weapon->glow.color), DRAW_BLEND_ADD,
        weapon->glow.scale);
}

void Gun_ApplyFlashSemiTransparency(void)
{
    // TR3+ level data already flags the flash faces as semi-transparent;
    // never touch it there.
    if (g_TRVersion >= 3) {
        return;
    }
    // The PS1 versions drew the muzzle flashes and the flare fire
    // semi-transparent; the PC data has them as plain opaque meshes.
    static const OBJECT_ID flash_objects[] = {
        O_GUN_FLASH,
        O_M16_FLASH,
        O_FLARE_FIRE,
    };
    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(flash_objects); i++) {
        Object_SetSemiTransparent(
            flash_objects[i], g_Config.visuals.enable_gun_glow);
    }
}

void Gun_FindTargetPoint(const ITEM *const item, GAME_VECTOR *const target)
{
    const BOUNDS_16 *const bounds = &Item_GetBestFrame(item)->bounds;
    const int32_t x = bounds->min.x + (bounds->max.x - bounds->min.x) / 2;
    const int32_t y = bounds->min.y + (bounds->max.y - bounds->min.y) / 3;
    const int32_t z = bounds->min.z + (bounds->max.z - bounds->min.z) / 2;
    target->pos = XYZ_32_OffsetLocalYaw(
        item->pos, (XYZ_32) { .x = x, .z = z }, item->rot.y);
    target->pos.y = item->pos.y + y;
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
        angles[0] >= weapon->lock.min_yaw && angles[0] <= weapon->lock.max_yaw
        && angles[1] >= weapon->lock.min_pitch
        && angles[1] <= weapon->lock.max_pitch) {
        lara->left_arm.lock = 1;
        lara->right_arm.lock = 1;
    } else {
        if (lara->left_arm.lock
            && (angles[0] < weapon->left_arm.min_yaw
                || angles[0] > weapon->left_arm.max_yaw
                || angles[1] < weapon->left_arm.min_pitch
                || angles[1] > weapon->left_arm.max_pitch)) {
            lara->left_arm.lock = 0;
        }
        if (lara->right_arm.lock
            && (angles[0] < weapon->right_arm.min_yaw
                || angles[0] > weapon->right_arm.max_yaw
                || angles[1] < weapon->right_arm.min_pitch
                || angles[1] > weapon->right_arm.max_pitch)) {
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
        const WEAPON_INFO *const info = Gun_Registry_Get(lara->gun_type);
        if (info->draw_meshes_func != nullptr) {
            info->draw_meshes_func(lara->gun_type);
        }
    }
}

void Gun_DrawFlash(
    const LARA_GUN_TYPE weapon_type, const CLIP clip, const bool interpolated)
{
    if (Gun_Registry_Get(weapon_type)->flash_is_optional
        && !g_Config.visuals.enable_shotgun_flash) {
        return;
    }

    const WEAPON_INFO *const info = Gun_Registry_Get(weapon_type);
    GUN_FLASH flash;
    if (info->flash_func != nullptr) {
        flash = info->flash_func();
    } else {
        flash = (GUN_FLASH) {
            .object_id = O_GUN_FLASH,
            .rot = { .x = -DEG_90, .z = 2 * Random_GetDraw() },
        };
    }
    const XYZ_16 rot = flash.rot;

    const WEAPON_INFO weapon = (*Gun_Registry_Get(weapon_type));
    if (interpolated) {
        Matrix_TranslateRel32_I(weapon.flash.pos.right);
        Matrix_RotX_I(rot.x);
        Matrix_RotY_I(rot.y);
        Matrix_RotZ_I(rot.z);
    } else {
        Matrix_TranslateRel32(weapon.flash.pos.right);
        Matrix_RotX(rot.x);
        Matrix_RotY(rot.y);
        Matrix_RotZ(rot.z);
    }

    const GAME_VECTOR pos = {
        .room_num = Lara_GetItem()->room_num,
        .pos = Matrix_MulVec32_M(g_WMatrixPtr, (XYZ_32) {}),
    };
    Output_Water_PushLaraMesh(WATER_LARA_MESH_OTHER, pos, 0);

    if (g_TRVersion < 3) {
        Output_CalculateStaticLight(weapon.flash.shade);
    } else {
        Output_CalculateStaticLightRGB_F(weapon.flash.color);
    }
    const OBJECT *const flash_obj = Object_Get(flash.object_id);
    if (flash_obj->loaded) {
        Object_DrawMesh(flash_obj->mesh_idx, clip, interpolated);
    }

    M_DrawGunGlow(&weapon, interpolated);
    Output_Water_PopLaraMesh();
}

void Gun_UpdateLaraMeshes(const OBJECT_ID obj_id)
{
    const INVENTORY_STATE *const inv = Inv_GetState();
    const bool lara_has_rifle = Gun_GetBackChoice(inv) != LGT_UNARMED;
    const bool lara_has_pistols = Gun_GetHolsterChoice(inv) != LGT_UNARMED;

    const LARA_GUN_TYPE picked_up = Gun_GetTypeForObject(obj_id);
    const STOW_PLACE stow_place = picked_up == LGT_UNARMED
        ? STOW_PLACE_NONE
        : Gun_Registry_Get(picked_up)->stow_place;

    LARA_GUN_TYPE back_gun_type = LGT_UNARMED;
    LARA_GUN_TYPE holsters_gun_type = LGT_UNARMED;

    if (!lara_has_rifle && stow_place == STOW_PLACE_BACK) {
        back_gun_type = picked_up;
    } else if (!lara_has_pistols && stow_place == STOW_PLACE_HOLSTER) {
        holsters_gun_type = picked_up;
    }

    if (back_gun_type != LGT_UNARMED) {
        Gun_SetLaraBackMesh(back_gun_type);
    }

    if (holsters_gun_type != LGT_UNARMED) {
        Gun_SetLaraHolsterLMesh(holsters_gun_type);
        Gun_SetLaraHolsterRMesh(holsters_gun_type);
    }
}

void Gun_HitTarget(
    ITEM *const item, const GAME_VECTOR *const start,
    const GAME_VECTOR *const hit_pos, int32_t damage)
{
    OBJECT *const obj = Object_Get(item->object_id);
    if (obj->gun_hit_func != nullptr) {
        const bool use_default =
            obj->gun_hit_func(item, start, hit_pos, &damage);
        if (!use_default) {
            return;
        }
    }

    const bool make_ricochet = !Item_ShouldSpawnBlood(item);
    if (item->object_id == O_SHIVA && make_ricochet) {
        damage = 0;
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    const bool was_alive = item->hit_points > 0;
    Item_TakeDamage(item, damage, IDF_NONE, Lara_GetItem());
    if (was_alive && item->hit_points <= 0
        && g_Config.gameplay.target_mode == TARGET_LOCK_MODE_SEMI) {
        lara->target = nullptr;
    }
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
            if (start != nullptr) {
                Spawn_RicochetRay(*start, pos, 3);
            } else {
                Spawn_Ricochet(pos);
            }
        } else {
            Spawn_Blood(
                hit_pos->x, hit_pos->y, hit_pos->z,
                g_TRVersion == 4 ? (Random_GetControl() & 3) + 3 : item->speed,
                item->rot.y, item->room_num);
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

        case O_SKATE_KID:
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
    const ITEM *const old_target = lara->target;

    // Preserve OG targeting behavior.
    if (g_Config.gameplay.target_mode == TARGET_LOCK_MODE_FULL
        && g_Config.gameplay.target_change_mode == TARGET_CHANGE_MODE_OFF
        && !g_Input.action) {
        lara->target = nullptr;
    }

    const GAME_VECTOR start = {
        .x = lara_item->pos.x,
        .y = lara_item->pos.y - 650,
        .z = lara_item->pos.z,
        .room_num = lara_item->room_num,
    };

    M_TARGET_CONTEXT ctx = {
        .weapon = weapon,
        .start = &start,
        .lara_item = lara_item,
        .lara = lara,
        .old_target = old_target,
        .max_dist = weapon->target_dist,
        .best_target = nullptr,
        .best_y_rot = INT16_MAX,
        .best_dist = INT32_MAX,
        .num_targets = 0,
        .old_target_dist = INT32_MAX,
        .old_target_y_rot = INT16_MAX,
        .old_target_in_list = false,
    };

    // First pass: active creatures
    for (int32_t i = 0; i < LOT_SLOT_COUNT; i++) {
        const CREATURE *const creature = LOT_GetBaddieSlot(i);
        if (creature->item_num == NO_ITEM) {
            continue;
        }
        M_ConsiderTarget(&ctx, Item_Get(creature->item_num));
    }

    // Second pass: other objects, including skidoo driver, whose targetable
    // ITEM is NOT in the active item list
    for (int32_t item_num = 0; item_num < Item_GetLevelCount(); item_num++) {
        ITEM *const item = Item_Get(item_num);
        if (M_TargetListContains(item, ctx.num_targets)) {
            continue;
        }
        M_ConsiderTarget(&ctx, item);
    }

    m_TargetCount = ctx.num_targets;
    m_BestTarget = ctx.best_target;

    if ((g_Config.gameplay.target_mode == TARGET_LOCK_MODE_FULL
         || g_Config.gameplay.target_mode == TARGET_LOCK_MODE_SEMI)
        && g_Input.action && lara->target != nullptr) {
        Gun_TargetInfo(weapon);
        return;
    }

    if (ctx.num_targets > 0) {
        bool found_current_target = false;
        for (int16_t slot = 0; slot < ctx.num_targets; slot++) {
            if (m_TargetList[slot] == lara->target) {
                found_current_target = true;
                break;
            }
        }

        if (!found_current_target) {
            lara->target = ctx.best_target;
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

    for (int16_t new_target = 0; new_target < m_TargetCount; new_target++) {
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

    // Every target in range has been through the cycle already: start it over
    // rather than leaving Lara with none, which would unlock her arms.
    if (lara->target == nullptr) {
        lara->target = m_BestTarget;
        m_LastTargetList[0] = nullptr;
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

void Gun_DrawFlashMirrored(const LARA_GUN_TYPE weapon_type, const CLIP clip)
{
    WEAPON_INFO *const weapon = Gun_Registry_Get(weapon_type);
    SWAP(weapon->flash.pos.right, weapon->flash.pos.left);
    Gun_DrawFlash(weapon_type, clip, false);
    SWAP(weapon->flash.pos.right, weapon->flash.pos.left);
}
