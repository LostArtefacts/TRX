#include "game/gun/gun_rifle.h"

#include <libtrx/config.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/gun.h>
#include <libtrx/game/gun/vars.h>
#include <libtrx/game/input.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/random.h>
#include <libtrx/game/sound.h>

#include <stdint.h>

#define GUN_RIFLE_EQUIP_ANIM 1
#define GUN_RIFLE_DRAW_FRAME 10
#define GUN_RIFLE_UNDRAW_FRAME 21

typedef enum {
    LA_G_AIM = 0,
    LA_G_DRAW = 1,
    LA_G_RECOIL = 2,
    LA_G_UNDRAW = 3,
    LA_G_UNAIM = 4,
    LA_G_RELOAD = 5,
    LA_G_UAIM = 6,
    LA_G_UUNAIM = 7,
    LA_G_URECOIL = 8,
    LA_G_SURF_UNDRAW = 9,
} LARA_GUN_ANIMATION;

static bool m_M16Firing = false;
static bool g_Gun_ReloadHarpoon = false; // TODO: make module-level

static void M_AnimateGun(ITEM *const item)
{
    // While the item is drawn in Lara_Draw, it needs a world position for
    // sound effect commands in Item_Animate.
    const ITEM *const lara_item = Lara_GetItem();
    item->pos.x = lara_item->pos.x;
    item->pos.y = lara_item->pos.y - LARA_HEIGHT;
    item->pos.z = lara_item->pos.z;
    Item_Animate(item);
}

void Gun_Rifle_Draw(const LARA_GUN_TYPE weapon_type)
{
    ITEM *item;

    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->gun_item_num != NO_ITEM) {
        item = Item_Get(lara->gun_item_num);
    } else {
        lara->gun_item_num = Item_Create();
        item = Item_Get(lara->gun_item_num);
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
        lara->right_arm.frame_base = obj->frame_base;
        lara->left_arm.frame_base = obj->frame_base;
    }

    M_AnimateGun(item);

    if (item->current_anim_state == LA_G_AIM
        || item->current_anim_state == LA_G_UAIM) {
        Gun_Rifle_Ready(weapon_type);
    } else if (Item_TestFrameEqual(item, GUN_RIFLE_DRAW_FRAME)) {
        Gun_Rifle_DrawMeshes(weapon_type);
    } else if (lara->water_status == LWS_UNDERWATER) {
        item->goal_anim_state = LA_G_UAIM;
    }

    lara->left_arm.anim_num = item->anim_num;
    lara->left_arm.frame_base = Item_GetAnim(item)->frame_ptr;
    lara->left_arm.frame_num = Item_GetRelativeFrame(item);
    lara->right_arm.anim_num = item->anim_num;
    lara->right_arm.frame_base = Item_GetAnim(item)->frame_ptr;
    lara->right_arm.frame_num = Item_GetRelativeFrame(item);
}

void Gun_Rifle_Undraw(const LARA_GUN_TYPE weapon_type)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();

    ITEM *const item = Item_Get(lara->gun_item_num);
    const ANIM *const anim = Item_GetAnim(item);
    if (lara->water_status == LWS_SURFACE
        && Anim_HasChange(anim, LA_G_SURF_UNDRAW)) {
        item->goal_anim_state = LA_G_SURF_UNDRAW;
    } else {
        item->goal_anim_state = LA_G_UNDRAW;
    }
    M_AnimateGun(item);

    if (item->status == IS_DEACTIVATED) {
        Item_Kill(lara->gun_item_num);
        lara->gun_item_num = NO_ITEM;
        lara->gun_status = LGS_ARMLESS;
        lara->target = nullptr;
        lara->left_arm.frame_num = 0;
        lara->left_arm.lock = 0;
        lara->right_arm.frame_num = 0;
        lara->right_arm.lock = 0;
    } else if (
        item->current_anim_state == LA_G_UNDRAW
        && Item_TestFrameEqual(item, GUN_RIFLE_UNDRAW_FRAME)) {
        Gun_Rifle_UndrawMeshes(weapon_type);
    }

    if (!g_Input.look || g_Config.gameplay.look_mode == LOOK_MODE_RESTRICTED) {
        lara->head_rot.x = 0;
        lara->head_rot.y = 0;
        lara->torso_rot.x += lara->torso_rot.x / -2;
        lara->torso_rot.y += lara->torso_rot.y / -2;
    }
    lara->left_arm.anim_num = item->anim_num;
    lara->left_arm.frame_base = Item_GetAnim(item)->frame_ptr;
    lara->left_arm.frame_num = Item_GetRelativeFrame(item);
    lara->right_arm.anim_num = item->anim_num;
    lara->right_arm.frame_base = Item_GetAnim(item)->frame_ptr;
    lara->right_arm.frame_num = Item_GetRelativeFrame(item);
}

void Gun_Rifle_Ready(const LARA_GUN_TYPE weapon_type)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->gun_status = LGS_READY;
    lara->target = nullptr;

    const OBJECT *const obj = Object_Get(Gun_GetWeaponAnim(weapon_type));
    lara->left_arm.frame_base = obj->frame_base;
    lara->left_arm.frame_num = LF_G_AIM_START;
    lara->left_arm.lock = 0;
    lara->left_arm.rot.x = 0;
    lara->left_arm.rot.y = 0;
    lara->left_arm.rot.z = 0;

    lara->right_arm.frame_base = obj->frame_base;
    lara->right_arm.frame_num = LF_G_AIM_START;
    lara->right_arm.lock = 0;
    lara->right_arm.rot.x = 0;
    lara->right_arm.rot.y = 0;
    lara->right_arm.rot.z = 0;

    if (g_Config.gameplay.look_mode == LOOK_MODE_RESTRICTED) {
        lara->head_rot.x = 0;
        lara->head_rot.y = 0;
        lara->torso_rot.x = 0;
        lara->torso_rot.y = 0;
    }
}

void Gun_Rifle_Control(const LARA_GUN_TYPE weapon_type)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    WEAPON_INFO *const weapon = &g_Weapons[weapon_type];

    Gun_GetNewTarget(weapon);
    if (g_InputDB.change_target && g_Config.gameplay.enable_target_change) {
        Gun_ChangeTarget(weapon);
    }

    Gun_AimWeapon(weapon, &lara->left_arm);

    if (lara->left_arm.lock) {
        lara->torso_rot.y = lara->left_arm.rot.y / 2;
        lara->torso_rot.x = lara->left_arm.rot.x / 2;
        if (g_Config.gameplay.look_mode != LOOK_MODE_UNRESTRICTED
            || g_Camera.type != CAM_LOOK) {
            lara->head_rot.x = 0;
            lara->head_rot.y = 0;
        }
    }

    Gun_Rifle_Animate(weapon_type);

    if (lara->right_arm.flash_gun) {
        Gun_AddDynamicLight();
    }
}

static void Gun_Rifle_AnimateOld(const LARA_GUN_TYPE weapon_type)
{
    const ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara = Lara_GetLaraInfo();
    int16_t ani = lara->left_arm.frame_num;

    if (lara->left_arm.lock) {
        if (Anim_TestAbsFrameRange(ani, LF_SG_AIM_START, LF_SG_AIM_END)) {
            ani++;
            if (Anim_TestAbsFrameEqual(ani, LF_SG_DRAW_START)) {
                ani = LF_SG_RECOIL_START;
            }
        } else if (Anim_TestAbsFrameEqual(ani, LF_SG_RECOIL_START)) {
            if (g_Input.action) {
                Gun_Rifle_Fire(weapon_type, false);
                ani++;
            }
        } else if (Anim_TestAbsFrameRange(
                       ani, LF_SG_RECOILING, LF_SG_RECOIL_END)) {
            ani++;
            if (Anim_TestAbsFrameEqual(ani, LF_SG_UNDRAW_START)) {
                ani = LF_SG_RECOIL_START;
            } else if (Anim_TestAbsFrameEqual(ani, LF_SG_RECOIL_SFX)) {
                Sound_Effect(SFX_LARA_RELOAD, &lara_item->pos, SPM_NORMAL);
            }
        } else if (Anim_TestAbsFrameRange(
                       ani, LF_SG_UNAIM_START, LF_SG_UNAIM_RAISE)) {
            ani++;
            if (Anim_TestAbsFrameEqual(ani, LF_SG_UNAIM_END)) {
                ani = LF_SG_AIM_START;
            }
        }
    } else if (
        g_Config.gameplay.fix_shotgun_targeting && lara->target != nullptr) {
        if (Anim_TestAbsFrameEqual(ani, LF_SG_RECOIL_START)) {
            ani = LF_SG_UNAIM_START;
            ani++;
        } else if (Anim_TestAbsFrameRange(
                       ani, LF_SG_RECOILING, LF_SG_RECOIL_END)) {
            ani++;
            if (Anim_TestAbsFrameEqual(ani, LF_SG_RECOIL_SFX)) {
                Sound_Effect(SFX_LARA_RELOAD, &lara_item->pos, SPM_NORMAL);
            } else if (Anim_TestAbsFrameEqual(ani, LF_SG_RECOIL_RESET_FIX)) {
                ani = LF_SG_AIM_START;
            } else if (Anim_TestAbsFrameEqual(ani, LF_SG_UNDRAW_START)) {
                ani = LF_SG_UNAIM_START;
            }
        } else if (Anim_TestAbsFrameRange(
                       ani, LF_SG_UNAIM_START, LF_SG_UNAIM_RAISE)) {
            ani++;
            if (Anim_TestAbsFrameEqual(ani, LF_SG_UNAIM_END)) {
                ani = LF_SG_AIM_START;
            }
        } else if (Anim_TestAbsFrameRange(
                       ani, LF_SG_AIM_BEND, LF_SG_DRAW_START)) {
            ani--;
        }
    } else {
        if (g_Input.action && Anim_TestAbsFrameEqual(ani, LF_SG_AIM_START)) {
            ani++;
        } else if (Anim_TestAbsFrameRange(ani, LF_SG_AIM_BEND, LF_SG_AIM_END)) {
            ani++;
            if (Anim_TestAbsFrameEqual(ani, LF_SG_DRAW_START)) {
                ani = g_Input.action ? LF_SG_RECOIL_START : LF_SG_UNAIM_START;
            }
        } else if (Anim_TestAbsFrameEqual(ani, LF_SG_RECOIL_START)) {
            if (g_Input.action) {
                Gun_Rifle_Fire(weapon_type, false);
                ani++;
            } else {
                ani = LF_SG_UNAIM_START;
            }
        } else if (Anim_TestAbsFrameRange(
                       ani, LF_SG_RECOILING, LF_SG_RECOIL_END)) {
            ani++;
            if (g_Config.gameplay.fix_shotgun_targeting) {
                if (Anim_TestAbsFrameEqual(ani, LF_SG_RECOIL_SFX)) {
                    Sound_Effect(SFX_LARA_RELOAD, &lara_item->pos, SPM_NORMAL);
                } else if (Anim_TestAbsFrameEqual(ani, LF_SG_UNDRAW_START)) {
                    ani =
                        g_Input.action ? LF_SG_RECOIL_START : LF_SG_UNAIM_START;
                }
            } else {
                if (Anim_TestAbsFrameEqual(ani, LF_SG_RECOIL_RESET_OG)) {
                    ani = LF_SG_AIM_START;
                } else if (Anim_TestAbsFrameEqual(ani, LF_SG_UNDRAW_START)) {
                    ani = LF_SG_UNAIM_START;
                } else if (Anim_TestAbsFrameEqual(ani, LF_SG_RECOIL_SFX)) {
                    Sound_Effect(SFX_LARA_RELOAD, &lara_item->pos, SPM_NORMAL);
                }
            }
        } else if (Anim_TestAbsFrameRange(
                       ani, LF_SG_UNAIM_START, LF_SG_UNAIM_RAISE)) {
            ani++;
            if (Anim_TestAbsFrameEqual(ani, LF_SG_UNAIM_END)) {
                ani = LF_SG_AIM_START;
            }
        }
    }
    lara->right_arm.frame_num = ani;
    lara->left_arm.frame_num = ani;
}

void Gun_Rifle_Animate(const LARA_GUN_TYPE weapon_type)
{
    const ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara = Lara_GetLaraInfo();

    const bool running = weapon_type == LGT_M16 && lara_item->speed != 0;
    ITEM *const item = Item_Get(lara->gun_item_num);

    switch (item->current_anim_state) {
    case LA_G_AIM:
        m_M16Firing = false;
        if (g_Gun_ReloadHarpoon) {
            item->goal_anim_state = LA_G_RELOAD;
            g_Gun_ReloadHarpoon = false;
        } else if (lara->water_status == LWS_UNDERWATER || running) {
            item->goal_anim_state = LA_G_UAIM;
        } else if (
            (g_Input.action && lara->target == nullptr)
            || lara->left_arm.lock) {
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
        } else if (lara->water_status != LWS_UNDERWATER && !running) {
            item->goal_anim_state = LA_G_AIM;
        } else if (
            (g_Input.action && lara->target == nullptr)
            || lara->left_arm.lock) {
            item->goal_anim_state = LA_G_URECOIL;
        } else {
            item->goal_anim_state = LA_G_UUNAIM;
        }
        break;

    case LA_G_RECOIL:
        if (Item_TestFrameEqual(item, 0)) {
            item->goal_anim_state = LA_G_UNAIM;
            if (lara->water_status != LWS_UNDERWATER && !running
                && !g_Gun_ReloadHarpoon) {
                if (g_Input.action) {
                    if (lara->target == nullptr || lara->left_arm.lock) {
                        Gun_Rifle_Fire(weapon_type, false);
                        if (weapon_type == LGT_M16) {
                            Sound_Effect(
                                SFX_M16_FIRE, &lara_item->pos, SPM_NORMAL);
                            m_M16Firing = true;
                        }
                        item->goal_anim_state = LA_G_RECOIL;
                    }
                } else if (lara->left_arm.lock) {
                    item->goal_anim_state = LA_G_AIM;
                }
            }

            if (item->goal_anim_state != LA_G_RECOIL && m_M16Firing) {
                Sound_Effect(SFX_M16_STOP, &lara_item->pos, SPM_NORMAL);
                m_M16Firing = false;
            }
        } else if (m_M16Firing) {
            Sound_Effect(SFX_M16_FIRE, &lara_item->pos, SPM_NORMAL);
        } else if (
            weapon_type == LGT_SHOTGUN && !g_Input.action
            && !lara->left_arm.lock) {
            item->goal_anim_state = LA_G_UNAIM;
        }
        break;

    case LA_G_URECOIL:
        if (Item_TestFrameEqual(item, 0)) {
            item->goal_anim_state = LA_G_UUNAIM;
            if ((lara->water_status == LWS_UNDERWATER || running)
                && !g_Gun_ReloadHarpoon) {
                if (g_Input.action) {
                    if (lara->target == nullptr || lara->left_arm.lock) {
                        Gun_Rifle_Fire(weapon_type, true);
                        item->goal_anim_state = LA_G_URECOIL;
                    }
                } else if (lara->left_arm.lock) {
                    item->goal_anim_state = LA_G_UAIM;
                }
            }
        }

        if (weapon_type == LGT_M16 && item->goal_anim_state == LA_G_URECOIL) {
            Sound_Effect(SFX_M16_FIRE, &lara_item->pos, SPM_NORMAL);
        }
        break;

    default:
        break;
    }

    M_AnimateGun(item);
    lara->left_arm.anim_num = item->anim_num;
    lara->left_arm.frame_base = Item_GetAnim(item)->frame_ptr;
    lara->left_arm.frame_num = Item_GetRelativeFrame(item);
    lara->right_arm.anim_num = item->anim_num;
    lara->right_arm.frame_base = Item_GetAnim(item)->frame_ptr;
    lara->right_arm.frame_num = Item_GetRelativeFrame(item);
}
