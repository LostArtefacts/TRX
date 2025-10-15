#include "game/gun/control.h"

#include "config.h"
#include "debug.h"
#include "game/camera.h"
#include "game/game.h"
#include "game/gun/common.h"
#include "game/gun/misc.h"
#include "game/gun/pistols.h"
#include "game/gun/rifle.h"
#include "game/gun/vars.h"
#include "game/input.h"
#include "game/inventory.h"
#include "game/lara.h"
#include "game/los.h"
#include "game/matrix.h"
#include "game/random.h"
#include "game/rooms.h"
#include "game/sound.h"
#include "game/spawn.h"
#include "game/stats.h"

static struct {
    LARA_GUN_TYPE gun_type;
    INPUT_ROLE input_role;
} m_QuicDrawKeys[] = {
    { .gun_type = LGT_PISTOLS, .input_role = INPUT_ROLE_EQUIP_PISTOLS },
    { .gun_type = LGT_SHOTGUN, .input_role = INPUT_ROLE_EQUIP_SHOTGUN },
    { .gun_type = LGT_MAGNUMS, .input_role = INPUT_ROLE_EQUIP_MAGNUMS },
    { .gun_type = LGT_UZIS, .input_role = INPUT_ROLE_EQUIP_UZIS },
#if TR_VERSION >= 2
    { .gun_type = LGT_HARPOON, .input_role = INPUT_ROLE_EQUIP_HARPOON },
    { .gun_type = LGT_M16, .input_role = INPUT_ROLE_EQUIP_M16 },
    { .gun_type = LGT_GRENADE,
      .input_role = INPUT_ROLE_EQUIP_GRENADE_LAUNCHER },
#endif
    { .gun_type = LGT_UNKNOWN, .input_role = (INPUT_ROLE)-1 },
};

static bool M_IsUsableUnderwater(const LARA_GUN_TYPE gun_type)
{
#if TR_VERSION == 1
    return false;
#else
    return gun_type == LGT_HARPOON;
#endif
}

static bool M_IsTooSubmerged(const LARA_GUN_TYPE gun_type)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    return lara->water_surface_dist > -g_Weapons[gun_type].gun_height;
}

static LARA_GUN_TYPE M_NeedToQuickDraw(void)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    for (int32_t i = 0; m_QuicDrawKeys[i].gun_type != LGT_UNKNOWN; i++) {
        if (Input_IsPressedDB(m_QuicDrawKeys[i].input_role)
            && Inv_RequestItem(Gun_GetGunObject(m_QuicDrawKeys[i].gun_type))
                > 0) {
            return m_QuicDrawKeys[i].gun_type;
        }
    }
    return LGT_UNKNOWN;
}

static bool M_QuickDrawWeapon(void)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    const LARA_GUN_TYPE gun_type = M_NeedToQuickDraw();
    if (gun_type != LGT_UNKNOWN) {
        lara->request_gun_type = gun_type;
        return true;
    }
    return false;
}

static bool M_CanEquip(void)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
#if TR_VERSION >= 2
    if (lara->request_gun_type == LGT_FLARE) {
        return true;
    }
#endif
    if (Lara_Vehicle_IsMounted()) {
        return false;
    }
    if (!Inv_RequestItem(Gun_GetGunObject(lara->request_gun_type))) {
        return false;
    }
    switch (lara->water_status) {
    case LWS_CHEAT:
        return false;
    case LWS_ABOVE_WATER:
        return true;
    case LWS_SURFACE:
    case LWS_UNDERWATER:
        return M_IsUsableUnderwater(lara->request_gun_type);
    case LWS_WADE: {
        if (M_IsUsableUnderwater(lara->request_gun_type)) {
            return true;
        }
        if (lara->gun_status == LGS_ARMLESS
            || M_IsTooSubmerged(lara->gun_type)) {
            return true;
        }
        return false;
    default:
        ASSERT_FAIL();
        return false;
    }
    }
}

static bool M_NeedToDraw(void)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (g_Input.draw || lara->request_gun_type != lara->gun_type) {
        return true;
    }
    return false;
}

static bool M_NeedToUndraw(void)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (g_Input.draw || lara->request_gun_type != lara->gun_type) {
        return true;
    }
    if (M_QuickDrawWeapon()) {
        if (g_Config.input.quick_guns_mode == QUICK_GUNS_DRAW_AND_HOLSTER
            || lara->request_gun_type != lara->gun_type) {
            return true;
        }
    }
    switch (lara->water_status) {
    case LWS_CHEAT:
        return true;
    case LWS_UNDERWATER:
    case LWS_SURFACE:
        return !M_IsUsableUnderwater(lara->request_gun_type);
    case LWS_ABOVE_WATER:
        return false;
    case LWS_WADE:
        return !M_IsUsableUnderwater(lara->request_gun_type)
            && !M_IsTooSubmerged(lara->gun_type);
    default:
        return false;
    }
}

static void M_DecideRequestedWeapon(void)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    const ITEM *const lara_item = Lara_GetItem();
    if (g_Input.draw) {
        LARA_GUN_TYPE requested_gun = lara->last_gun_type != LGT_UNARMED
            ? lara->last_gun_type
            : LGT_PISTOLS;
        if (Inv_RequestItem(Gun_GetGunObject(requested_gun)) == 0) {
            for (LARA_GUN_TYPE gun = 0; gun < NUM_WEAPONS; gun++) {
                if (Inv_RequestItem(Gun_GetGunObject(gun)) > 0) {
                    requested_gun = gun;
                    break;
                }
            }
        }
        if (Inv_RequestItem(Gun_GetGunObject(requested_gun)) != 0) {
            lara->request_gun_type = requested_gun;
        }
        return;
    }

#if TR_VERSION >= 2
    if (g_Input.use_flare) {
        if (lara->gun_type == LGT_FLARE) {
            lara->gun_status = LGS_UNDRAW;
        } else if (
            Inv_RequestItem(O_FLARES_ITEM)
            && (!g_Config.gameplay.fix_free_flare_glitch
                || lara_item->current_anim_state != LS(LS_PICKUP))) {
            lara->request_gun_type = LGT_FLARE;
        }
    }
#endif
}

static void M_DrawRequestedWeapon(void)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (M_CanEquip()) {
#if TR_VERSION >= 2
        if (lara->gun_type == LGT_FLARE) {
            Lara_Flare_Dispose(false);
        }
#endif
        lara->gun_type = lara->request_gun_type;
        Gun_InitialiseNewWeapon();
        lara->gun_status = LGS_DRAW;
        lara->right_arm.frame_num = 0;
        lara->left_arm.frame_num = 0;
    } else {
#if TR_VERSION >= 2
        if (lara->request_gun_type != LGT_FLARE
            && lara->request_gun_type != LGT_UNARMED) {
            lara->last_gun_type = lara->request_gun_type;
        }
        if (lara->gun_type == LGT_FLARE) {
            lara->request_gun_type = LGT_FLARE;
        } else {
            lara->gun_type = lara->request_gun_type;
        }
#else
        if (lara->request_gun_type != LGT_UNARMED) {
            lara->last_gun_type = lara->request_gun_type;
        }
        lara->gun_type = lara->request_gun_type;
#endif
    }
}

static void M_TryUndrawWeapon(void)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
#if TR_VERSION >= 2
    if (g_Input.use_flare && Inv_RequestItem(O_FLARES_ITEM)) {
        lara->request_gun_type = LGT_FLARE;
    }
#endif
    if (M_NeedToUndraw()) {
        lara->gun_status = LGS_UNDRAW;
    }
}

static void M_UpdateGunState(void)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    const ITEM *const lara_item = Lara_GetItem();
    if (lara_item->hit_points <= 0) {
        lara->gun_status = LGS_ARMLESS;
    } else if (lara->gun_status == LGS_ARMLESS) {
        if (M_QuickDrawWeapon()) {
            M_DrawRequestedWeapon();
        } else {
            M_DecideRequestedWeapon();
            if (M_NeedToDraw()) {
                M_DrawRequestedWeapon();
            }
        }
    } else if (lara->gun_status == LGS_READY) {
        M_TryUndrawWeapon();
    } else {
        M_QuickDrawWeapon();
    }
}

void Gun_Control(void)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    const ITEM *const lara_item = Lara_GetItem();

    if (lara->extra_anim && lara->gun_status != LGS_HANDS_BUSY) {
        lara->request_gun_type = LGT_UNARMED;
        return;
    }

    if (lara->left_arm.flash_gun > 0) {
        lara->left_arm.flash_gun--;
    }
    if (lara->right_arm.flash_gun > 0) {
        lara->right_arm.flash_gun--;
    }

    M_UpdateGunState();

    switch (lara->gun_status) {
    case LGS_ARMLESS:
    case LGS_HANDS_BUSY:
#if TR_VERSION >= 2
        if (lara->gun_type == LGT_FLARE) {
            Lara_Flare_Control();
        }
#endif
        break;

    case LGS_DRAW:
#if TR_VERSION >= 2
        if (lara->gun_type != LGT_FLARE && lara->gun_type != LGT_UNARMED) {
            lara->last_gun_type = lara->gun_type;
        }
#else
        if (lara->gun_type != LGT_UNARMED) {
            lara->last_gun_type = lara->gun_type;
        }
#endif

        switch (lara->gun_type) {
        case LGT_PISTOLS:
        case LGT_MAGNUMS:
        case LGT_UZIS:
            if (g_Camera.type != CAM_CINEMATIC && g_Camera.type != CAM_LOOK) {
                g_Camera.type = CAM_COMBAT;
            }
            Gun_Pistols_Draw(lara->gun_type);
            break;

        case LGT_SHOTGUN:
#if TR_VERSION >= 2
        case LGT_M16:
        case LGT_GRENADE:
        case LGT_HARPOON:
#endif
            if (g_Camera.type != CAM_CINEMATIC && g_Camera.type != CAM_LOOK) {
                g_Camera.type = CAM_COMBAT;
            }
            Gun_Rifle_Draw(lara->gun_type);
            break;

#if TR_VERSION >= 2
        case LGT_FLARE:
            Lara_Flare_Draw();
            break;
#endif

        default:
            lara->gun_status = LGS_ARMLESS;
            break;
        }
        break;

    case LGS_UNDRAW:
        Lara_Mesh_SwapSingle(LM_HEAD, O_LARA);

        switch (lara->gun_type) {
        case LGT_PISTOLS:
        case LGT_MAGNUMS:
        case LGT_UZIS:
            Gun_Pistols_Undraw(lara->gun_type);
            break;

        case LGT_SHOTGUN:
#if TR_VERSION >= 2
        case LGT_M16:
        case LGT_GRENADE:
        case LGT_HARPOON:
#endif
            Gun_Rifle_Undraw(lara->gun_type);
            break;

#if TR_VERSION >= 2
        case LGT_FLARE:
            Lara_Flare_Undraw();
            break;
#endif

        default:
            return;
        }
        break;

    case LGS_READY:
        if (lara->pistol_ammo.ammo && g_Input.action) {
            Lara_Mesh_SwapSingle(LM_HEAD, O_LARA_UZIS);
        } else {
            Lara_Mesh_SwapSingle(LM_HEAD, O_LARA);
        }

        if (g_Camera.type != CAM_CINEMATIC && g_Camera.type != CAM_LOOK) {
            g_Camera.type = CAM_COMBAT;
        }

        if (g_Input.action) {
            AMMO_INFO *const ammo = Gun_GetAmmoInfo(lara->gun_type);
            ASSERT(ammo != nullptr);

            if (ammo->ammo <= 0) {
                ammo->ammo = 0;
#if TR_VERSION >= 2
                Sound_Effect(SFX_CLICK, &lara_item->pos, SPM_NORMAL);
#endif
                lara->request_gun_type =
                    Inv_RequestItem(O_PISTOL_ITEM) ? LGT_PISTOLS : LGT_UNARMED;
                break;
            }
        }

        switch (lara->gun_type) {
        case LGT_PISTOLS:
        case LGT_MAGNUMS:
        case LGT_UZIS:
            Gun_Pistols_Control(lara->gun_type);
            break;

        case LGT_SHOTGUN:
#if TR_VERSION >= 2
        case LGT_M16:
        case LGT_GRENADE:
        case LGT_HARPOON:
#endif
            Gun_Rifle_Control(lara->gun_type);
            break;

        default:
            return;
        }
        break;

#if TR_VERSION >= 2
    case LGS_SPECIAL:
        Lara_Flare_Draw();
        break;
#endif

    default:
        return;
    }
}

int32_t Gun_FireWeapon(
    const LARA_GUN_TYPE weapon_type, ITEM *const target, const ITEM *const src,
    const int16_t *const angles)
{
    const WEAPON_INFO *const weapon = &g_Weapons[weapon_type];
    LARA_INFO *const lara = Lara_GetLaraInfo();

    AMMO_INFO *const ammo = Gun_GetAmmoInfo(weapon_type);
    ASSERT(ammo != nullptr);

    if (ammo == &lara->pistol_ammo || Game_IsBonusFlagSet(GBF_NGPLUS)) {
        ammo->ammo = 1000;
    }
    if (ammo->ammo <= 0) {
        ammo->ammo = 0;
#if TR_VERSION == 1
        Sound_Effect(SFX_LARA_EMPTY, &src->pos, SPM_NORMAL);
        if (Inv_RequestItem(O_PISTOL_ITEM)) {
            lara->request_gun_type = LGT_PISTOLS;
        } else {
            lara->gun_status = LGS_UNDRAW;
        }
#endif
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
            + weapon->shot_accuracy * (Random_GetControl() - DEG_90) / DEG_360,
        .y = angles[0]
            + weapon->shot_accuracy * (Random_GetControl() - DEG_90) / DEG_360,
        .z = 0,
    };
    Matrix_GenerateW2V(&view_pos, &view_rot);

    SPHERE spheres[33];
    int32_t sphere_count = Collide_GetSpheres(target, spheres, false);
    int32_t best_sphere = -1;
    int32_t best_dist = INT32_MAX;

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

    GAME_VECTOR start = {
        .pos = view_pos,
        .room_num = src->room_num,
    };

    if (best_sphere < 0) {
        const int32_t dist = weapon->target_dist;
        GAME_VECTOR hit_pos = {
#if TR_VERSION == 1
            .x = start.x + g_MatrixPtr->_20,
            .y = start.y + g_MatrixPtr->_21,
            .z = start.z + g_MatrixPtr->_22,
#else
            .x = start.x + ((dist * g_MatrixPtr->_20) >> W2V_SHIFT),
            .y = start.y + ((dist * g_MatrixPtr->_21) >> W2V_SHIFT),
            .z = start.z + ((dist * g_MatrixPtr->_22) >> W2V_SHIFT),
#endif
            .room_num = start.room_num,
        };
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
        GAME_VECTOR hit_pos = {
            .x = start.x + ((best_dist * g_MatrixPtr->_20) >> W2V_SHIFT),
            .y = start.y + ((best_dist * g_MatrixPtr->_21) >> W2V_SHIFT),
            .z = start.z + ((best_dist * g_MatrixPtr->_22) >> W2V_SHIFT),
            .room_num = src->room_num,
        };
        Room_GetSector(hit_pos.x, hit_pos.y, hit_pos.z, &hit_pos.room_num);
        Gun_SmashItems(start.pos, hit_pos.pos, nullptr);
        Gun_HitTarget(
            target, &hit_pos,
            weapon->damage * (Game_IsBonusFlagSet(GBF_JAPANESE) ? 2 : 1));
        return 1;
    }
}
