#include "game/gun/gun.h"

#include "game/gun/gun_misc.h"
#include "game/gun/gun_pistols.h"
#include "game/gun/gun_rifle.h"
#include "game/input.h"
#include "game/inventory.h"
#include "game/lara.h"
#include "game/sound.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/lara.h>

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
    return gun_type == LGT_HARPOON;
}

static bool M_IsTooSubmerged(const LARA_GUN_TYPE gun_type)
{
    return g_Lara.water_surface_dist > -g_Weapons[gun_type].gun_height;
}

static bool M_CanEquip(void)
{
    if (g_Lara.request_gun_type == LGT_FLARE) {
        return true;
    }
    if (Lara_Vehicle_IsMounted()) {
        return false;
    }
    switch (g_Lara.water_status) {
    case LWS_CHEAT:
        return false;
    case LWS_ABOVE_WATER:
        return true;
    case LWS_UNDERWATER:
        return M_IsUsableUnderwater(g_Lara.request_gun_type);
    case LWS_WADE: {
        if (M_IsUsableUnderwater(g_Lara.request_gun_type)) {
            return true;
        }
        if (g_Lara.gun_status == LGS_ARMLESS
            || M_IsTooSubmerged(g_Lara.gun_type)) {
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
    if (g_Input.draw || g_Lara.request_gun_type != g_Lara.gun_type) {
        return true;
    }
    return false;
}

static bool M_NeedToUndraw(void)
{
    if (g_Input.draw || g_Lara.request_gun_type != g_Lara.gun_type) {
        return true;
    }
    switch (g_Lara.water_status) {
    case LWS_CHEAT:
        return true;
    case LWS_UNDERWATER:
        return !M_IsUsableUnderwater(g_Lara.request_gun_type);
    case LWS_ABOVE_WATER:
        return false;
    case LWS_WADE:
        return !M_IsTooSubmerged(g_Lara.gun_type);
    default:
        return false;
    }
}

static void M_DecideRequestedWeapon(void)
{
    if (g_Input.draw) {
        g_Lara.request_gun_type = g_Lara.last_gun_type;
    } else if (g_InputDB.use_flare) {
        if (g_Lara.gun_type == LGT_FLARE) {
            g_Lara.gun_status = LGS_UNDRAW;
        } else if (
            Inv_RequestItem(O_FLARES_ITEM)
            && (!g_Config.gameplay.fix_free_flare_glitch
                || g_LaraItem->current_anim_state != LS_PICKUP)) {
            g_Lara.request_gun_type = LGT_FLARE;
        }
    }
}

static void M_TryDrawRequestedWeapon(void)
{
    if (M_NeedToDraw()) {
        if (M_CanEquip()) {
            if (g_Lara.gun_type == LGT_FLARE) {
                Lara_Flare_Dispose(false);
            }
            g_Lara.gun_type = g_Lara.request_gun_type;
            Gun_InitialiseNewWeapon();
            g_Lara.gun_status = LGS_DRAW;
            g_Lara.right_arm.frame_num = 0;
            g_Lara.left_arm.frame_num = 0;
        } else {
            g_Lara.last_gun_type = g_Lara.request_gun_type;
            if (g_Lara.gun_type == LGT_FLARE) {
                g_Lara.request_gun_type = LGT_FLARE;
            } else {
                g_Lara.gun_type = g_Lara.request_gun_type;
            }
        }
    }
}

static void M_TryUndrawWeapon(void)
{
    if (g_InputDB.use_flare && Inv_RequestItem(O_FLARES_ITEM)) {
        g_Lara.request_gun_type = LGT_FLARE;
    }
    if (M_NeedToUndraw()) {
        g_Lara.gun_status = LGS_UNDRAW;
    }
}

static void M_UpdateGunState(void)
{
    if (g_LaraItem->hit_points <= 0) {
        g_Lara.gun_status = LGS_ARMLESS;
    } else if (g_Lara.gun_status == LGS_ARMLESS) {
        M_DecideRequestedWeapon();
        M_TryDrawRequestedWeapon();
    } else if (g_Lara.gun_status == LGS_READY) {
        M_TryUndrawWeapon();
    }
}

void Gun_Control(void)
{
    if (g_Lara.extra_anim && g_Lara.gun_status != LGS_HANDS_BUSY) {
        g_Lara.request_gun_type = LGT_UNARMED;
        return;
    }

    if (g_Lara.left_arm.flash_gun > 0) {
        g_Lara.left_arm.flash_gun--;
    }
    if (g_Lara.right_arm.flash_gun > 0) {
        g_Lara.right_arm.flash_gun--;
    }

    M_UpdateGunState();

    switch (g_Lara.gun_status) {
    case LGS_ARMLESS:
    case LGS_HANDS_BUSY:
        if (g_Lara.gun_type == LGT_FLARE) {
            Lara_Flare_Control();
        }
        break;

    case LGS_DRAW:
        if (g_Lara.gun_type != LGT_FLARE && g_Lara.gun_type != LGT_UNARMED) {
            g_Lara.last_gun_type = g_Lara.gun_type;
        }

        switch (g_Lara.gun_type) {
        case LGT_PISTOLS:
        case LGT_MAGNUMS:
        case LGT_UZIS:
            if (g_Camera.type != CAM_CINEMATIC && g_Camera.type != CAM_LOOK) {
                g_Camera.type = CAM_COMBAT;
            }
            Gun_Pistols_Draw(g_Lara.gun_type);
            break;

        case LGT_SHOTGUN:
        case LGT_M16:
        case LGT_GRENADE:
        case LGT_HARPOON:
            if (g_Camera.type != CAM_CINEMATIC && g_Camera.type != CAM_LOOK) {
                g_Camera.type = CAM_COMBAT;
            }
            Gun_Rifle_Draw(g_Lara.gun_type);
            break;

        case LGT_FLARE:
            Lara_Flare_Draw();
            break;

        default:
            g_Lara.gun_status = LGS_ARMLESS;
            break;
        }
        break;

    case LGS_UNDRAW:
        Lara_Mesh_SwapSingle(LM_HEAD, O_LARA);

        switch (g_Lara.gun_type) {
        case LGT_PISTOLS:
        case LGT_MAGNUMS:
        case LGT_UZIS:
            Gun_Pistols_Undraw(g_Lara.gun_type);
            break;

        case LGT_SHOTGUN:
        case LGT_M16:
        case LGT_GRENADE:
        case LGT_HARPOON:
            Gun_Rifle_Undraw(g_Lara.gun_type);
            break;

        case LGT_FLARE:
            Lara_Flare_Undraw();
            break;
        default:
            return;
        }
        break;

    case LGS_READY:
        if (g_Lara.pistol_ammo.ammo && g_Input.action) {
            Lara_Mesh_SwapSingle(LM_HEAD, O_LARA_UZIS);
        } else {
            Lara_Mesh_SwapSingle(LM_HEAD, O_LARA);
        }

        if (g_Camera.type != CAM_CINEMATIC && g_Camera.type != CAM_LOOK) {
            g_Camera.type = CAM_COMBAT;
        }

        if (g_Input.action) {
            AMMO_INFO *const ammo = Gun_GetAmmoInfo(g_Lara.gun_type);
            ASSERT(ammo != nullptr);

            if (ammo->ammo <= 0) {
                ammo->ammo = 0;
                Sound_Effect(SFX_CLICK, &g_LaraItem->pos, SPM_NORMAL);
                g_Lara.request_gun_type =
                    Inv_RequestItem(O_PISTOL_ITEM) ? LGT_PISTOLS : LGT_UNARMED;
                break;
            }
        }

        switch (g_Lara.gun_type) {
        case LGT_PISTOLS:
        case LGT_MAGNUMS:
        case LGT_UZIS:
            Gun_Pistols_Control(g_Lara.gun_type);
            break;

        case LGT_SHOTGUN:
        case LGT_M16:
        case LGT_GRENADE:
        case LGT_HARPOON:
            Gun_Rifle_Control(g_Lara.gun_type);
            break;

        default:
            return;
        }
        break;

    case LGS_SPECIAL:
        Lara_Flare_Draw();
        break;

    default:
        return;
    }
}

void Gun_InitialiseNewWeapon(void)
{
    g_Lara.left_arm.flash_gun = 0;
    g_Lara.left_arm.frame_num = 0;
    g_Lara.left_arm.lock = 0;
    g_Lara.left_arm.rot.x = 0;
    g_Lara.left_arm.rot.y = 0;
    g_Lara.left_arm.rot.z = 0;
    g_Lara.right_arm.flash_gun = 0;
    g_Lara.right_arm.frame_num = 0;
    g_Lara.right_arm.lock = 0;
    g_Lara.right_arm.rot.x = 0;
    g_Lara.right_arm.rot.y = 0;
    g_Lara.right_arm.rot.z = 0;
    g_Lara.target = nullptr;

    switch (g_Lara.gun_type) {
    case LGT_PISTOLS:
    case LGT_MAGNUMS:
    case LGT_UZIS: {
        const OBJECT *const obj = Object_Get(O_LARA_PISTOLS);
        g_Lara.left_arm.frame_base = obj->frame_base;
        g_Lara.right_arm.frame_base = obj->frame_base;
        if (g_Lara.gun_status != LGS_ARMLESS) {
            Gun_Pistols_DrawMeshes(g_Lara.gun_type);
        }
        break;
    }

    case LGT_SHOTGUN:
    case LGT_M16:
    case LGT_GRENADE:
    case LGT_HARPOON: {
        const OBJECT *const obj =
            Object_Get(Gun_GetWeaponAnim(g_Lara.gun_type));
        g_Lara.left_arm.frame_base = obj->frame_base;
        g_Lara.right_arm.frame_base = obj->frame_base;
        if (g_Lara.gun_status != LGS_ARMLESS) {
            Gun_Rifle_DrawMeshes(g_Lara.gun_type);
        }
        break;
    }

    case LGT_FLARE: {
        const OBJECT *const obj = Object_Get(O_LARA_FLARE);
        g_Lara.left_arm.frame_base = obj->frame_base;
        g_Lara.right_arm.frame_base = obj->frame_base;
        if (g_Lara.gun_status != LGS_ARMLESS) {
            Lara_Flare_DrawMeshes();
        }
        break;
    }

    default:
        const ANIM *const anim = Item_GetAnim(g_LaraItem);
        g_Lara.left_arm.frame_base = anim->frame_ptr;
        g_Lara.right_arm.frame_base = anim->frame_ptr;
        break;
    }
}

void Gun_UpdateLaraMeshes(const GAME_OBJECT_ID obj_id)
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
    } else if (!lara_has_rifle && obj_id == O_HARPOON_ITEM) {
        back_gun_type = LGT_HARPOON;
    } else if (!lara_has_rifle && obj_id == O_M16_ITEM) {
        back_gun_type = LGT_M16;
    } else if (!lara_has_rifle && obj_id == O_GRENADE_ITEM) {
        back_gun_type = LGT_GRENADE;
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
