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

static bool M_IsUsableUnderwater(LARA_GUN_TYPE gun_type);
static bool M_IsTooSubmerged(LARA_GUN_TYPE gun_type);
static LARA_GUN_TYPE M_NeedToQuickDraw(void);
static bool M_QuickDrawWeapon(void);
static bool M_CanEquip(void);
static bool M_NeedToDraw(void);
static bool M_NeedToUndraw(void);
static void M_DecideRequestedWeapon(void);
static void M_DrawRequestedWeapon(void);
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
#if TR_VERSION == 1
        if (lara->request_gun_type != lara->gun_type) {
            return true;
        }
#else
        if (g_Config.input.quick_guns_mode == QUICK_GUNS_DRAW_AND_HOLSTER
            || lara->request_gun_type != lara->gun_type) {
            return true;
        }
#endif
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
    if (g_Input.use_flare) {
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

void Gun_UpdateGunState(void)
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
