#include "game/gun/rifle.h"

#include "config.h"
#include "game/game.h"
#include "game/gun/common.h"
#include "game/gun/const.h"
#include "game/gun/control.h"
#include "game/gun/misc.h"
#include "game/gun/vars.h"
#include "game/lara.h"
#include "game/random.h"
#include "game/sound.h"
#include "game/stats.h"

#if TR_VERSION == 2
// TODO: remove these externs
extern bool g_Gun_ReloadHarpoon;
extern void Lara_GetJointAbsPosition(XYZ_32 *vec, LARA_MESH joint);
#endif

static void M_FireGeneric(const LARA_GUN_TYPE weapon_type)
{
    const ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara = Lara_GetLaraInfo();
    bool fired = false;
    int16_t angles[2] = {
        lara->left_arm.rot.y + lara_item->rot.y,
        lara->left_arm.rot.x,
    };

    for (int32_t i = 0; i < SHOTGUN_AMMO_CLIP; i++) {
        int16_t dangles[2] = {
            angles[0]
                + SHOTGUN_PELLET_SCATTER * (Random_GetControl() - 0x4000)
                    / 0x10000,
            angles[1]
                + SHOTGUN_PELLET_SCATTER * (Random_GetControl() - 0x4000)
                    / 0x10000,
        };
        if (Gun_FireWeapon(weapon_type, lara->target, lara_item, dangles)) {
            fired = true;
        }
    }

    if (fired) {
        lara->right_arm.flash_gun = g_Weapons[weapon_type].flash_time;
        Sound_Effect(
            g_Weapons[weapon_type].sample_num, &lara_item->pos, SPM_NORMAL);
    }
}

#if TR_VERSION == 2
static void M_FireM16(const bool running)
{
    const ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara = Lara_GetLaraInfo();
    int16_t angles[2] = {
        lara->left_arm.rot.y + lara_item->rot.y,
        lara->left_arm.rot.x,
    };

    if (g_Config.gameplay.fix_m16_accuracy) {
        if (running) {
            g_Weapons[LGT_M16].shot_accuracy = DEG_1 * 12;
            g_Weapons[LGT_M16].damage = 1;
        } else {
            g_Weapons[LGT_M16].shot_accuracy = DEG_1 * 4;
            g_Weapons[LGT_M16].damage = 3;
        }
    }

    if (Gun_FireWeapon(LGT_M16, lara->target, lara_item, angles)) {
        lara->right_arm.flash_gun = g_Weapons[LGT_M16].flash_time;
    }
}

static void M_FireHarpoon(void)
{
    const ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->harpoon_ammo.ammo <= 0) {
        goto finish;
    }

    const int16_t item_num = Item_Create();
    if (item_num == NO_ITEM) {
        goto finish;
    }

    const WEAPON_INFO *const weapon = &g_Weapons[LGT_HARPOON];
    const XYZ_32 origin = {
        .x = lara_item->pos.x,
        .y = lara_item->pos.y - weapon->gun_height,
        .z = lara_item->pos.z,
    };

    ITEM *const projectile_item = Item_Get(item_num);
    projectile_item->object_id = O_HARPOON_BOLT;
    projectile_item->room_num = lara_item->room_num;

    XYZ_32 offset = {
        .x = -2,
        .y = 373,
        .z = 77,
    };

    Lara_GetJointAbsPosition(&offset, LM_HAND_R);
    projectile_item->pos.x = offset.x;
    projectile_item->pos.y = offset.y;
    projectile_item->pos.z = offset.z;
    Item_Initialise(item_num);

    if (lara->target != nullptr) {
        GAME_VECTOR lara_vec;
        Gun_FindTargetPoint(lara->target, &lara_vec);
        const int32_t dx = lara_vec.pos.x - projectile_item->pos.x;
        const int32_t dz = lara_vec.pos.z - projectile_item->pos.z;
        const int32_t dy = lara_vec.pos.y - projectile_item->pos.y;
        const int32_t dxz = Math_Sqrt(SQUARE(dx) + SQUARE(dz));
        projectile_item->rot.y = Math_Atan(dz, dx);
        projectile_item->rot.x = -Math_Atan(dxz, dy);
        projectile_item->rot.z = 0;
    } else {
        projectile_item->rot.x = lara->left_arm.rot.x + lara_item->rot.x;
        projectile_item->rot.y = lara->left_arm.rot.y + lara_item->rot.y;
        projectile_item->rot.z = 0;
    }

    projectile_item->fall_speed =
        (-HARPOON_BOLT_SPEED * Math_Sin(projectile_item->rot.x)) >> W2V_SHIFT;
    projectile_item->speed =
        (HARPOON_BOLT_SPEED * Math_Cos(projectile_item->rot.x)) >> W2V_SHIFT;
    Item_AddActive(item_num);
    projectile_item->status = IS_ACTIVE;

    Gun_SmashItems(origin, projectile_item->pos, nullptr);

    lara->harpoon_ammo.ammo--;
    Stats_AddAmmoUsed();

finish:
    const int32_t recoil = g_Config.gameplay.harpoon_recoil;
    const bool is_ngplus = Game_IsBonusFlagSet(GBF_NGPLUS);
    if (recoil <= 0) {
        if (is_ngplus) {
            lara->harpoon_ammo.ammo++;
        }
    } else if ((lara->harpoon_ammo.ammo % recoil) == 0) {
        g_Gun_ReloadHarpoon = true;
        if (is_ngplus) {
            lara->harpoon_ammo.ammo += recoil;
        }
    }
}

static void M_FireGrenade(void)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    const ITEM *const lara_item = Lara_GetItem();
    if (lara->grenade_ammo.ammo <= 0) {
        return;
    }
    const WEAPON_INFO *const weapon = &g_Weapons[LGT_GRENADE];
    const XYZ_32 origin = {
        .x = lara_item->pos.x,
        .y = lara_item->pos.y - weapon->gun_height,
        .z = lara_item->pos.z,
    };

    const int16_t item_num = Item_Create();
    if (item_num == NO_ITEM) {
        return;
    }

    ITEM *const projectile_item = Item_Get(item_num);
    projectile_item->object_id = O_GRENADE;
    projectile_item->room_num = lara_item->room_num;

    XYZ_32 offset = {
        .x = -2,
        .y = 373,
        .z = 77,
    };
    Lara_GetJointAbsPosition(&offset, LM_HAND_R);
    projectile_item->pos.x = offset.x;
    projectile_item->pos.y = offset.y;
    projectile_item->pos.z = offset.z;
    Item_Initialise(item_num);

    projectile_item->rot.x = lara->left_arm.rot.x + lara_item->rot.x;
    projectile_item->rot.y = lara->left_arm.rot.y + lara_item->rot.y;
    projectile_item->rot.z = 0;
    projectile_item->speed = GRENADE_SPEED;
    projectile_item->fall_speed = 0;
    Item_AddActive(item_num);
    projectile_item->status = IS_ACTIVE;

    Gun_SmashItems(origin, projectile_item->pos, nullptr);

    if (!Game_IsBonusFlagSet(GBF_NGPLUS)) {
        lara->grenade_ammo.ammo--;
    }
    Stats_AddAmmoUsed();
}
#endif

void Gun_Rifle_Fire(const LARA_GUN_TYPE weapon_type, const bool running)
{
    switch (weapon_type) {
#if TR_VERSION >= 2
    case LGT_HARPOON:
        M_FireHarpoon();
        break;
    case LGT_GRENADE:
        if (!running) {
            M_FireGrenade();
        }
        break;
    case LGT_M16:
        M_FireM16(running);
        break;
#endif
    default:
        if (!running) {
            M_FireGeneric(weapon_type);
        }
        break;
    }
}

void Gun_Rifle_DrawMeshes(const LARA_GUN_TYPE weapon_type)
{
    Gun_SetLaraHandLMesh(LGT_UNARMED);
    Gun_SetLaraHandRMesh(weapon_type);
    Gun_SetLaraBackMesh(LGT_UNARMED);
}

void Gun_Rifle_UndrawMeshes(const LARA_GUN_TYPE weapon_type)
{
    Gun_SetLaraHandLMesh(LGT_UNARMED);
    Gun_SetLaraHandRMesh(LGT_UNARMED);
    Gun_SetLaraBackMesh(weapon_type);
}
