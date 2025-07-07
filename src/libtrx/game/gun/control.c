#include "game/gun/control.h"

#include "config.h"
#include "debug.h"
#include "game/camera.h"
#include "game/gun/common.h"
#include "game/gun/vars.h"
#include "game/input.h"
#include "game/inventory.h"
#include "game/lara.h"
#include "game/sound.h"

static bool M_IsUsableUnderwater(LARA_GUN_TYPE gun_type);
static bool M_IsTooSubmerged(LARA_GUN_TYPE gun_type);
static bool M_CanEquip(void);
static bool M_NeedToDraw(void);
static bool M_NeedToUndraw(void);
static void M_DecideRequestedWeapon(void);
static void M_TryDrawRequestedWeapon(void);
static void M_TryUndrawWeapon(void);

static bool M_IsUsableUnderwater(const LARA_GUN_TYPE gun_type)
{
#if TR_VERSION == 1
    return true;
#else
    return gun_type == LGT_HARPOON;
#endif
}

static bool M_IsTooSubmerged(const LARA_GUN_TYPE gun_type)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    return lara->water_surface_dist > -g_Weapons[gun_type].gun_height;
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
    switch (lara->water_status) {
    case LWS_CHEAT:
        return false;
    case LWS_ABOVE_WATER:
        return true;
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
        return true;
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
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (g_Input.draw || lara->request_gun_type != lara->gun_type) {
        return true;
    }
    switch (lara->water_status) {
    case LWS_CHEAT:
        return true;
    case LWS_UNDERWATER:
        return !M_IsUsableUnderwater(lara->request_gun_type);
    case LWS_ABOVE_WATER:
        return false;
    case LWS_WADE:
        return !M_IsTooSubmerged(lara->gun_type);
    default:
        return false;
    }
}

static void M_DecideRequestedWeapon(void)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    const ITEM *const lara_item = Lara_GetItem();
    if (g_Input.draw) {
        lara->request_gun_type = lara->last_gun_type;
        return;
    }
#if TR_VERSION >= 2
    if (g_InputDB.use_flare) {
        if (lara->gun_type == LGT_FLARE) {
            lara->gun_status = LGS_UNDRAW;
        } else if (
            Inv_RequestItem(O_FLARES_ITEM)
            && (!g_Config.gameplay.fix_free_flare_glitch
                || lara_item->current_anim_state != LS_PICKUP)) {
            lara->request_gun_type = LGT_FLARE;
        }
    }
#endif
}

static void M_TryDrawRequestedWeapon(void)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (M_NeedToDraw()) {
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
            lara->last_gun_type = lara->request_gun_type;
#if TR_VERSION >= 2
            if (lara->gun_type == LGT_FLARE) {
                lara->request_gun_type = LGT_FLARE;
            } else {
                lara->gun_type = lara->request_gun_type;
            }
#else
            lara->gun_type = lara->request_gun_type;
#endif
        }
    }
}

static void M_TryUndrawWeapon(void)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
#if TR_VERSION >= 2
    if (g_InputDB.use_flare && Inv_RequestItem(O_FLARES_ITEM)) {
        lara->request_gun_type = LGT_FLARE;
    }
#endif
    if (M_NeedToUndraw()) {
        lara->gun_status = LGS_UNDRAW;
    }
}

void Gun_UpdateGunState(void)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    const ITEM *const lara_item = Lara_GetItem();
    if (lara_item->hit_points <= 0) {
        lara->gun_status = LGS_ARMLESS;
    } else if (lara->gun_status == LGS_ARMLESS) {
        M_DecideRequestedWeapon();
        M_TryDrawRequestedWeapon();
    } else if (lara->gun_status == LGS_READY) {
        M_TryUndrawWeapon();
    }
}
