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
#include "game/random.h"

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

    const GAME_OBJECT_ID anim_type = Gun_GetLaraAnim(lara->gun_type);
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

void Gun_UpdateLaraMeshes(const GAME_OBJECT_ID obj_id)
{
#if TR_VERSION == 1
    const bool lara_has_rifle = Inv_RequestItem(O_SHOTGUN_ITEM);
#else
    const bool lara_has_rifle = Inv_RequestItem(O_SHOTGUN_ITEM)
        || Inv_RequestItem(O_HARPOON_ITEM) || Inv_RequestItem(O_M16_ITEM)
        || Inv_RequestItem(O_GRENADE_ITEM);
#endif
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
