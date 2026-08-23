#include <trx/game/gun/control.h>

#include <trx/config.h>
#include <trx/debug.h>
#include <trx/game/camera.h>
#include <trx/game/collision/los.h>
#include <trx/game/game.h>
#include <trx/game/gun/common.h>
#include <trx/game/gun/flare.h>
#include <trx/game/gun/misc.h>
#include <trx/game/gun/pistols.h>
#include <trx/game/gun/registry.h>
#include <trx/game/gun/rifle.h>
#include <trx/game/gun/smashing.h>
#include <trx/game/gun/smoke.h>
#include <trx/game/input.h>
#include <trx/game/inventory.h>
#include <trx/game/lara.h>
#include <trx/game/matrix.h>
#include <trx/game/objects/common.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>
#include <trx/game/spawn.h>
#include <trx/game/stats.h>
#include <trx/version.h>

static const LARA_TRX_STATE m_CrawlStates[] = {
    // clang-format off
    LS_CRAWL_IDLE,
    LS_CRAWL_FORWARD,
    LS_CRAWL_BACK,
    LS_CRAWL_TURN_LEFT,
    LS_CRAWL_TURN_RIGHT,
    LS_CRAWL_TO_CLIMB,
    LS_TRX_INVALID, // sentinel
    // clang-format on
};

// What Lara reaches for when the weapon in her hands has ran out of ammo:
// the default weapon, so long as she carries it and it has ammo left.
static LARA_GUN_TYPE M_GetFallbackGunType(void)
{
    const LARA_GUN_TYPE gun_type = Gun_GetDefaultType();
    return Inv_HasItem(Gun_GetGunObject(gun_type))
            && Gun_HasRoundsLeft(gun_type)
        ? gun_type
        : LGT_UNARMED;
}

static void M_CheckSmashablesBehindTarget(
    const ITEM *const target, const GAME_VECTOR start,
    const GAME_VECTOR hit_pos, const int32_t max_dist)
{
    if (target == nullptr || target->object_id != O_SOPHIA) {
        return;
    }

    // OG does a raycast instead of segment cast when checking for smashables.
    // TRX normally doesn't do that, but in the Reunion battle against Sophia,
    // Sophia stands directly in front of the Fuse Box, preventing Lara from
    // shooting her which is a breaking behavior.
    //
    // This function does additional smashable pass by emulating the OG raycast.
    const int32_t hit_dist = XYZ_32_GetDistance(start.pos, hit_pos.pos);
    if (hit_dist >= max_dist) {
        return;
    }

    const int32_t offset = STEP_L / 16;
    GAME_VECTOR follow_start = {
        .x = hit_pos.x + ((offset * g_MatrixPtr->_20) >> W2V_SHIFT),
        .y = hit_pos.y + ((offset * g_MatrixPtr->_21) >> W2V_SHIFT),
        .z = hit_pos.z + ((offset * g_MatrixPtr->_22) >> W2V_SHIFT),
        .room_num = hit_pos.room_num,
    };
    Room_GetSector(follow_start.pos, &follow_start.room_num);

    GAME_VECTOR follow_end = {
        .x = start.x + ((max_dist * g_MatrixPtr->_20) >> W2V_SHIFT),
        .y = start.y + ((max_dist * g_MatrixPtr->_21) >> W2V_SHIFT),
        .z = start.z + ((max_dist * g_MatrixPtr->_22) >> W2V_SHIFT),
        .room_num = start.room_num,
    };
    Room_GetSector(follow_end.pos, &follow_end.room_num);

    Gun_SmashItems(follow_start, follow_end, nullptr, NO_OBJECT);
}

static bool M_IsUsableUnderwater(const LARA_GUN_TYPE gun_type)
{
    const WEAPON_INFO *const info = Gun_Registry_Get(gun_type);
    return info->is_usable_underwater;
}

// Where Lara is deep enough that only an underwater weapon can come out.
// Wading is not such a place: it takes the rest of them under conditions of
// its own.
static bool M_IsOnlyUnderwaterUsable(void)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    return lara->water_status == LWS_UNDERWATER
        || lara->water_status == LWS_SURFACE;
}

// Where the water stands below the muzzle, so that the weapon still fires.
static bool M_IsAboveWaterLine(const LARA_GUN_TYPE gun_type)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    return lara->water_surface_dist > -Gun_Registry_Get(gun_type)->gun_height;
}

static LARA_GUN_TYPE M_NeedToQuickDraw(void)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    for (int32_t i = 0; i < Gun_Registry_GetCount(); i++) {
        const WEAPON_INFO *const weapon = Gun_Registry_GetByIndex(i);
        const LARA_GUN_TYPE gun_type = weapon->gun_type;
        const INPUT_ROLE role = weapon->equip_input_role;
        if ((int32_t)role < 0) {
            continue;
        }
        if (Input_IsPressedDB(role)
            && Inv_HasItem(Gun_GetGunObject(gun_type))) {
            return gun_type;
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
    if (Gun_IsFlareType(lara->request_gun_type)) {
        return !Gun_IsFlareType(lara->gun_type);
    }
    if (lara->is_crouched && Gun_IsRifleType(lara->request_gun_type)) {
        return false;
    }
    if (Lara_Vehicle_IsMounted()) {
        return false;
    }
    if (!Inv_HasItem(Gun_GetGunObject(lara->request_gun_type))) {
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
    case LWS_WADE:
        return M_IsUsableUnderwater(lara->request_gun_type)
            || M_IsAboveWaterLine(lara->request_gun_type);
    default:
        ASSERT_FAIL();
        return false;
    }
}

static bool M_HasWeaponAnim(const LARA_GUN_TYPE gun_type)
{
    const OBJECT *const obj = Object_Get(Gun_GetLaraAnim(gun_type));
    return obj->loaded && obj->frame_base != nullptr;
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
        if (g_Config.input.quick_guns_mode == QUICK_GUNS_MODE_DRAW_AND_HOLSTER
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
            && !M_IsAboveWaterLine(lara->gun_type);
    default:
        return false;
    }
}

static bool M_IsLaraCrawling(void)
{
    return Lara_HasState(m_CrawlStates);
}

// All games refuse Lara to light a flare from the hotkey input while on all
// fours, but only TR4 onwards make her say so.
static bool M_IsFlareInputBlocked(void)
{
    return g_TRVersion >= 4 && M_IsLaraCrawling();
}

static void M_DecideRequestedWeapon(void)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    const ITEM *const lara_item = Lara_GetItem();
    if (g_Input.draw) {
        LARA_GUN_TYPE requested_gun = lara->last_gun_type != LGT_UNARMED
            ? lara->last_gun_type
            : Gun_GetDefaultType();
        if (g_Config.gameplay.enable_underwater_auto_draw
            && M_IsOnlyUnderwaterUsable()
            && !M_IsUsableUnderwater(requested_gun)) {
            for (int32_t i = 0; i < Gun_Registry_GetCount(); i++) {
                const LARA_GUN_TYPE gun = Gun_Registry_GetByIndex(i)->gun_type;
                if (M_IsUsableUnderwater(gun)
                    && Inv_HasItem(Gun_GetGunObject(gun))) {
                    requested_gun = gun;
                    break;
                }
            }
        }
        if (!Inv_HasItem(Gun_GetGunObject(requested_gun))) {
            for (int32_t i = 0; i < Gun_Registry_GetCount(); i++) {
                const LARA_GUN_TYPE gun = Gun_Registry_GetByIndex(i)->gun_type;
                if (Inv_HasItem(Gun_GetGunObject(gun))) {
                    requested_gun = gun;
                    break;
                }
            }
        }
        if (Inv_HasItem(Gun_GetGunObject(requested_gun))) {
            lara->request_gun_type = requested_gun;
        }
        return;
    }

    if (g_Input.use_flare && M_IsFlareInputBlocked()) {
        return;
    }

    if (g_Input.use_flare) {
        if (Gun_IsFlareType(lara->gun_type)) {
            lara->gun_status = LGS_UNDRAW;
        } else if (
            Inv_HasItem(O_FLAREBOX_ITEM)
            && (!g_Config.gameplay.fix_free_flare_glitch
                || lara_item->current_anim_state != LS(LS_PICKUP))) {
            lara->request_gun_type = Gun_GetFlareType();
        }
    }
}

static void M_DrawRequestedWeapon(void)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (M_CanEquip()) {
        if (!M_HasWeaponAnim(lara->request_gun_type)) {
            lara->request_gun_type = LGT_UNARMED;
            lara->gun_type = LGT_UNARMED;
            lara->gun_status = LGS_ARMLESS;
            return;
        }

        if (Gun_IsFlareType(lara->gun_type)) {
            Gun_Flare_Dispose(false);
        }

        lara->gun_type = lara->request_gun_type;
        Gun_InitialiseNewWeapon();
        lara->gun_status = LGS_DRAW;
        lara->right_arm.frame_num = 0;
        lara->left_arm.frame_num = 0;
    } else {
        if (!Gun_IsFlareType(lara->request_gun_type)
            && lara->request_gun_type != LGT_UNARMED) {
            lara->last_gun_type = lara->request_gun_type;
        }
        if (Gun_IsFlareType(lara->gun_type)) {
            lara->request_gun_type = lara->gun_type;
        } else {
            lara->gun_type = lara->request_gun_type;
        }
    }
}

static void M_TryUndrawWeapon(void)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (g_Input.use_flare && Inv_HasItem(O_FLAREBOX_ITEM)) {
        lara->request_gun_type = Gun_GetFlareType();
    }
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

static void M_RequestCombatCamera(void)
{
    if (g_Camera.type != CAM_CINEMATIC && g_Camera.type != CAM_LOOK
        && !Camera_IsLocked(g_Camera.num)) {
        g_Camera.type = CAM_COMBAT;
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

    Gun_Smoke_Control();

    // Crawling holds the hands busy, so the refusal has to be answered here
    // rather than in M_DecideRequestedWeapon, which armless Lara alone reaches.
    if (g_InputDB.use_flare && M_IsFlareInputBlocked()) {
        Sound_Effect(SFX_LARA_NO, nullptr, SPM_ALWAYS);
    }

    // Selecting a flare from the inventory while crawling will cache the
    // requested gun type, resulting in a free ghost flare when she stands up.
    if (Gun_IsFlareType(lara->request_gun_type)
        && !Gun_IsFlareType(lara->gun_type)
        && g_Config.gameplay.fix_free_flare_glitch && M_IsLaraCrawling()) {
        lara->request_gun_type = LGT_UNARMED;
        return;
    }

    M_UpdateGunState();

    switch (lara->gun_status) {
    case LGS_ARMLESS:
    case LGS_HANDS_BUSY: {
        const WEAPON_INFO *const info = Gun_Registry_Get(lara->gun_type);
        if (info->control_func != nullptr) {
            info->control_func(lara->gun_type, lara->gun_status);
        }
        break;
    }

    case LGS_DRAW: {
        const WEAPON_INFO *const info = Gun_Registry_Get(lara->gun_type);
        if (info->draw_func == nullptr) {
            lara->gun_status = LGS_ARMLESS;
            break;
        }
        if (info->is_remembered) {
            lara->last_gun_type = lara->gun_type;
        }
        if (info->wants_combat_camera) {
            M_RequestCombatCamera();
        }
        info->draw_func(lara->gun_type);
        break;
    }

    case LGS_UNDRAW: {
        Lara_Skin_SetCombatFace(false);
        const WEAPON_INFO *const info = Gun_Registry_Get(lara->gun_type);
        if (info->undraw_func == nullptr) {
            return;
        }
        info->undraw_func(lara->gun_type);
        break;
    }

    case LGS_READY:
        const bool has_rounds = Gun_HasRoundsLeft(lara->gun_type);
        Lara_Skin_SetCombatFace(has_rounds && g_Input.action);
        M_RequestCombatCamera();

        if (g_Input.action) {
            if (!has_rounds) {
                Inv_SetAmmo(lara->gun_type, 0);
                if (g_TRVersion >= 2) {
                    Sound_Effect(SFX_CLICK, &lara_item->pos, SPM_NORMAL);
                }
                lara->request_gun_type = M_GetFallbackGunType();
                break;
            }
        }

        const WEAPON_INFO *const info = Gun_Registry_Get(lara->gun_type);
        if (info->control_func == nullptr) {
            return;
        }
        info->control_func(lara->gun_type, lara->gun_status);
        break;

    case LGS_SPECIAL:
        Gun_Flare_Draw();
        break;

    default:
        return;
    }
}

void Gun_EnsureReady(void)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->gun_status == LGS_READY && Gun_IsRifleType(lara->gun_type)) {
        Gun_Rifle_EnsureReady(lara->gun_type);
    }
}

int32_t Gun_FireWeapon(
    const LARA_GUN_TYPE weapon_type, ITEM *const target, const ITEM *const src,
    const int16_t *const angles)
{
    const WEAPON_INFO *const weapon = Gun_Registry_Get(weapon_type);
    LARA_INFO *const lara = Lara_GetLaraInfo();

    ASSERT(Inv_HasAmmoSlot(weapon_type));

    if (!Gun_HasRoundsLeft(weapon_type)) {
        Inv_SetAmmo(weapon_type, 0);
        if (g_TRVersion == 1) {
            Sound_Effect(SFX_LARA_EMPTY, &src->pos, SPM_NORMAL);
            const LARA_GUN_TYPE fallback = M_GetFallbackGunType();
            if (fallback == LGT_UNARMED) {
                lara->gun_status = LGS_UNDRAW;
            } else {
                lara->request_gun_type = fallback;
            }
        }
        return 0;
    }
    Gun_SpendRound(weapon_type);
    Stats_AddAmmoUsed();
    lara->has_fired = true;

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
        GAME_VECTOR hit_pos = g_TRVersion == 1
            ? (GAME_VECTOR) {
                .x = start.x + g_MatrixPtr->_20,
                .y = start.y + g_MatrixPtr->_21,
                .z = start.z + g_MatrixPtr->_22,
                .room_num = start.room_num,
            }
            : (GAME_VECTOR) {
                .x = start.x + ((dist * g_MatrixPtr->_20) >> W2V_SHIFT),
                .y = start.y + ((dist * g_MatrixPtr->_21) >> W2V_SHIFT),
                .z = start.z + ((dist * g_MatrixPtr->_22) >> W2V_SHIFT),
                .room_num = start.room_num,
            };
        Room_GetSector(hit_pos.pos, &hit_pos.room_num);
        const bool object_on_los = LOS_Check(&start, &hit_pos, true);
        if (Gun_SmashItems(start, hit_pos, &hit_pos.pos, NO_OBJECT)
            == PROJECTILE_HIT_STOP) {
            Room_GetSector(hit_pos.pos, &hit_pos.room_num);
        }
        if (!object_on_los) {
            Spawn_RicochetRay(start, hit_pos, 8);
        }
        return -1;
    }

    Stats_AddAmmoHits();
    GAME_VECTOR hit_pos = {
        .x = start.x + ((best_dist * g_MatrixPtr->_20) >> W2V_SHIFT),
        .y = start.y + ((best_dist * g_MatrixPtr->_21) >> W2V_SHIFT),
        .z = start.z + ((best_dist * g_MatrixPtr->_22) >> W2V_SHIFT),
        .room_num = start.room_num,
    };
    Room_GetSector(hit_pos.pos, &hit_pos.room_num);
    Gun_SmashItems(start, hit_pos, nullptr, NO_OBJECT);
    Gun_HitTarget(target, &start, &hit_pos, weapon->damage);
    M_CheckSmashablesBehindTarget(target, start, hit_pos, weapon->target_dist);
    return 1;
}
