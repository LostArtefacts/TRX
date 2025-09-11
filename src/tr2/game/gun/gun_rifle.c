#include "game/gun/gun_rifle.h"

#include "game/gun/gun.h"
#include "game/gun/gun_misc.h"
#include "game/lara/misc.h"
#include "game/stats.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/game.h>
#include <libtrx/game/input.h>
#include <libtrx/game/lara/const.h>
#include <libtrx/game/math.h>
#include <libtrx/game/random.h>
#include <libtrx/game/sound.h>
#include <libtrx/utils.h>

#define GUN_RIFLE_EQUIP_ANIM 1
#define GUN_RIFLE_DRAW_FRAME 10
#define GUN_RIFLE_UNDRAW_FRAME 21

static bool m_M16Firing = false;
bool g_Gun_ReloadHarpoon = false; // TODO: make module-level

static void M_AnimateGun(ITEM *item);

static void M_AnimateGun(ITEM *const item)
{
    // While the item is drawn in Lara_Draw, it needs a world position for
    // sound effect commands in Item_Animate.
    item->pos.x = g_LaraItem->pos.x;
    item->pos.y = g_LaraItem->pos.y - LARA_HEIGHT;
    item->pos.z = g_LaraItem->pos.z;
    Item_Animate(item);
}

void Gun_Rifle_Ready(const LARA_GUN_TYPE weapon_type)
{
    g_Lara.gun_status = LGS_READY;
    g_Lara.target = nullptr;

    const OBJECT *const obj = Object_Get(Gun_GetWeaponAnim(weapon_type));
    g_Lara.left_arm.frame_base = obj->frame_base;
    g_Lara.left_arm.frame_num = LF_G_AIM_START;
    g_Lara.left_arm.lock = 0;
    g_Lara.left_arm.rot.x = 0;
    g_Lara.left_arm.rot.y = 0;
    g_Lara.left_arm.rot.z = 0;

    g_Lara.right_arm.frame_base = obj->frame_base;
    g_Lara.right_arm.frame_num = LF_G_AIM_START;
    g_Lara.right_arm.lock = 0;
    g_Lara.right_arm.rot.x = 0;
    g_Lara.right_arm.rot.y = 0;
    g_Lara.right_arm.rot.z = 0;

    if (g_Config.gameplay.look_mode == LOOK_MODE_RESTRICTED) {
        g_Lara.head_rot.x = 0;
        g_Lara.head_rot.y = 0;
        g_Lara.torso_rot.x = 0;
        g_Lara.torso_rot.y = 0;
    }
}

void Gun_Rifle_Control(const LARA_GUN_TYPE weapon_type)
{
    const WEAPON_INFO *const winfo = &g_Weapons[weapon_type];

    if (g_Input.action) {
        Gun_TargetInfo(winfo);
    } else {
        g_Lara.target = nullptr;
    }

    if (g_Lara.target == nullptr) {
        Gun_GetNewTarget(winfo);
    }

    Gun_AimWeapon(winfo, &g_Lara.left_arm);

    if (g_Lara.left_arm.lock) {
        g_Lara.head_rot.x = 0;
        g_Lara.head_rot.y = 0;
        g_Lara.torso_rot.x = g_Lara.left_arm.rot.x;
        g_Lara.torso_rot.y = g_Lara.left_arm.rot.y;
    }

    Gun_Rifle_Animate(weapon_type);

    if (g_Lara.right_arm.flash_gun
        && (weapon_type == LGT_SHOTGUN || weapon_type == LGT_M16)) {
        Gun_AddDynamicLight();
    }
}

void Gun_Rifle_Draw(const LARA_GUN_TYPE weapon_type)
{
    ITEM *item;
    if (g_Lara.gun_item_num != NO_ITEM) {
        item = Item_Get(g_Lara.gun_item_num);
    } else {
        g_Lara.gun_item_num = Item_Create();
        item = Item_Get(g_Lara.gun_item_num);
        item->object_id = Gun_GetWeaponAnim(weapon_type);
        if (weapon_type == LGT_GRENADE) {
            Item_SwitchToObjAnim(item, 0, 0, O_LARA_GRENADE);
        } else {
            Item_SwitchToAnim(item, GUN_RIFLE_EQUIP_ANIM, 0);
        }
        item->goal_anim_state = LA_G_DRAW;
        item->current_anim_state = LA_G_DRAW;
        item->status = IS_ACTIVE;
        item->room_num = NO_ROOM;
        const OBJECT *const obj = Object_Get(item->object_id);
        g_Lara.right_arm.frame_base = obj->frame_base;
        g_Lara.left_arm.frame_base = obj->frame_base;
    }
    M_AnimateGun(item);

    if (item->current_anim_state == LA_G_AIM
        || item->current_anim_state == LA_G_UAIM) {
        Gun_Rifle_Ready(weapon_type);
    } else if (Item_TestFrameEqual(item, GUN_RIFLE_DRAW_FRAME)) {
        Gun_Rifle_DrawMeshes(weapon_type);
    } else if (g_Lara.water_status == LWS_UNDERWATER) {
        item->goal_anim_state = LA_G_UAIM;
    }

    g_Lara.left_arm.anim_num = item->anim_num;
    g_Lara.left_arm.frame_base = Item_GetAnim(item)->frame_ptr;
    g_Lara.left_arm.frame_num = Item_GetRelativeFrame(item);
    g_Lara.right_arm.anim_num = item->anim_num;
    g_Lara.right_arm.frame_base = Item_GetAnim(item)->frame_ptr;
    g_Lara.right_arm.frame_num = Item_GetRelativeFrame(item);
}

void Gun_Rifle_Undraw(const LARA_GUN_TYPE weapon_type)
{
    ITEM *const item = Item_Get(g_Lara.gun_item_num);
    if (g_Lara.water_status == LWS_SURFACE) {
        item->goal_anim_state = LA_G_SURF_UNDRAW;
    } else {
        item->goal_anim_state = LA_G_UNDRAW;
    }
    M_AnimateGun(item);

    if (item->status == IS_DEACTIVATED) {
        Item_Kill(g_Lara.gun_item_num);
        g_Lara.gun_item_num = NO_ITEM;
        g_Lara.gun_status = LGS_ARMLESS;
        g_Lara.target = nullptr;
        g_Lara.left_arm.frame_num = 0;
        g_Lara.left_arm.lock = 0;
        g_Lara.right_arm.frame_num = 0;
        g_Lara.right_arm.lock = 0;
    } else if (
        item->current_anim_state == LA_G_UNDRAW
        && Item_TestFrameEqual(item, GUN_RIFLE_UNDRAW_FRAME)) {
        Gun_Rifle_UndrawMeshes(weapon_type);
    }

    if (!g_Input.look || g_Config.gameplay.look_mode == LOOK_MODE_RESTRICTED) {
        g_Lara.head_rot.x = 0;
        g_Lara.head_rot.y = 0;
        g_Lara.torso_rot.x += g_Lara.torso_rot.x / -2;
        g_Lara.torso_rot.y += g_Lara.torso_rot.y / -2;
    }
    g_Lara.left_arm.anim_num = item->anim_num;
    g_Lara.left_arm.frame_base = Item_GetAnim(item)->frame_ptr;
    g_Lara.left_arm.frame_num = Item_GetRelativeFrame(item);
    g_Lara.right_arm.anim_num = item->anim_num;
    g_Lara.right_arm.frame_base = Item_GetAnim(item)->frame_ptr;
    g_Lara.right_arm.frame_num = Item_GetRelativeFrame(item);
}

void Gun_Rifle_Animate(const LARA_GUN_TYPE weapon_type)
{
    const bool running = weapon_type == LGT_M16 && g_LaraItem->speed != 0;
    ITEM *const item = Item_Get(g_Lara.gun_item_num);

    switch (item->current_anim_state) {
    case LA_G_AIM:
        m_M16Firing = false;
        if (g_Gun_ReloadHarpoon) {
            item->goal_anim_state = LA_G_RELOAD;
            g_Gun_ReloadHarpoon = false;
        } else if (g_Lara.water_status == LWS_UNDERWATER || running) {
            item->goal_anim_state = LA_G_UAIM;
        } else if (
            (g_Input.action && g_Lara.target == nullptr)
            || g_Lara.left_arm.lock) {
            item->goal_anim_state = LA_G_RECOIL;
        } else {
            item->goal_anim_state = LA_G_UNAIM;
        }
        break;

    case LA_G_UAIM:
        m_M16Firing = false;
        if (g_Gun_ReloadHarpoon) {
            item->goal_anim_state = LA_G_RELOAD;
            g_Gun_ReloadHarpoon = false;
        } else if (g_Lara.water_status != LWS_UNDERWATER && !running) {
            item->goal_anim_state = LA_G_AIM;
        } else if (
            (g_Input.action && g_Lara.target == nullptr)
            || g_Lara.left_arm.lock) {
            item->goal_anim_state = LA_G_URECOIL;
        } else {
            item->goal_anim_state = LA_G_UUNAIM;
        }
        break;

    case LA_G_RECOIL:
        if (Item_TestFrameEqual(item, 0)) {
            item->goal_anim_state = LA_G_UNAIM;
            if (g_Lara.water_status != LWS_UNDERWATER && !running
                && !g_Gun_ReloadHarpoon) {
                if (g_Input.action) {
                    if (g_Lara.target == nullptr || g_Lara.left_arm.lock) {
                        Gun_Rifle_Fire(weapon_type, false);
                        if (weapon_type == LGT_M16) {
                            Sound_Effect(
                                SFX_M16_FIRE, &g_LaraItem->pos, SPM_NORMAL);
                            m_M16Firing = true;
                        }
                        item->goal_anim_state = LA_G_RECOIL;
                    }
                } else if (g_Lara.left_arm.lock) {
                    item->goal_anim_state = LA_G_AIM;
                }
            }

            if (item->goal_anim_state != LA_G_RECOIL && m_M16Firing) {
                Sound_Effect(SFX_M16_STOP, &g_LaraItem->pos, SPM_NORMAL);
                m_M16Firing = false;
            }
        } else if (m_M16Firing) {
            Sound_Effect(SFX_M16_FIRE, &g_LaraItem->pos, SPM_NORMAL);
        } else if (
            weapon_type == LGT_SHOTGUN && !g_Input.action
            && !g_Lara.left_arm.lock) {
            item->goal_anim_state = LA_G_UNAIM;
        }
        break;

    case LA_G_URECOIL:
        if (Item_TestFrameEqual(item, 0)) {
            item->goal_anim_state = LA_G_UUNAIM;
            if ((g_Lara.water_status == LWS_UNDERWATER || running)
                && !g_Gun_ReloadHarpoon) {
                if (g_Input.action) {
                    if (g_Lara.target == nullptr || g_Lara.left_arm.lock) {
                        Gun_Rifle_Fire(weapon_type, true);
                        item->goal_anim_state = LA_G_URECOIL;
                    }
                } else if (g_Lara.left_arm.lock) {
                    item->goal_anim_state = LA_G_UAIM;
                }
            }
        }

        if (weapon_type == LGT_M16 && item->goal_anim_state == LA_G_URECOIL) {
            Sound_Effect(SFX_M16_FIRE, &g_LaraItem->pos, SPM_NORMAL);
        }
        break;

    default:
        break;
    }

    M_AnimateGun(item);
    g_Lara.left_arm.anim_num = item->anim_num;
    g_Lara.left_arm.frame_base = Item_GetAnim(item)->frame_ptr;
    g_Lara.left_arm.frame_num = Item_GetRelativeFrame(item);
    g_Lara.right_arm.anim_num = item->anim_num;
    g_Lara.right_arm.frame_base = Item_GetAnim(item)->frame_ptr;
    g_Lara.right_arm.frame_num = Item_GetRelativeFrame(item);
}
