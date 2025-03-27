#include "game/gun/gun_rifle.h"

#include "game/gun/gun.h"
#include "game/gun/gun_misc.h"
#include "game/input.h"
#include "game/items.h"
#include "game/lara/misc.h"
#include "game/random.h"
#include "game/room.h"
#include "game/sound.h"
#include "game/stats.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/lara/const.h>
#include <libtrx/game/math.h>
#include <libtrx/utils.h>

#define GUN_RIFLE_EQUIP_ANIM 1
#define GUN_RIFLE_DRAW_FRAME 10
#define GUN_RIFLE_UNDRAW_FRAME 21

static bool m_M16Firing = false;
static bool m_HarpoonFired = false;

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

void Gun_Rifle_DrawMeshes(const LARA_GUN_TYPE weapon_type)
{
    Gun_SetLaraHandRMesh(weapon_type);
    g_Lara.back_gun = O_LARA;
}

void Gun_Rifle_UndrawMeshes(const LARA_GUN_TYPE weapon_type)
{
    Gun_SetLaraHandRMesh(LGT_UNARMED);
    g_Lara.back_gun = Gun_GetWeaponAnim(weapon_type);
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

void Gun_Rifle_FireShotgun(void)
{
    bool fired = false;

    int16_t angles[2];
    angles[0] = g_Lara.left_arm.rot.y + g_LaraItem->rot.y;
    angles[1] = g_Lara.left_arm.rot.x;

    for (int32_t i = 0; i < SHOTGUN_AMMO_CLIP; i++) {
        int16_t dangles[2];
        dangles[0] = angles[0]
            + SHOTGUN_PELLET_SCATTER * (Random_GetControl() - 0x4000) / 0x10000;
        dangles[1] = angles[1]
            + SHOTGUN_PELLET_SCATTER * (Random_GetControl() - 0x4000) / 0x10000;
        if (Gun_FireWeapon(LGT_SHOTGUN, g_Lara.target, g_LaraItem, dangles)) {
            fired = true;
        }
    }

    if (fired) {
        g_Lara.right_arm.flash_gun = g_Weapons[LGT_SHOTGUN].flash_time;
        Sound_Effect(
            g_Weapons[LGT_SHOTGUN].sample_num, &g_LaraItem->pos, SPM_NORMAL);
    }
}

void Gun_Rifle_FireM16(const bool running)
{
    int16_t angles[2];
    angles[0] = g_Lara.left_arm.rot.y + g_LaraItem->rot.y;
    angles[1] = g_Lara.left_arm.rot.x;

    if (g_Config.gameplay.fix_m16_accuracy) {
        if (running) {
            g_Weapons[LGT_M16].shot_accuracy = DEG_1 * 12;
            g_Weapons[LGT_M16].damage = 1;
        } else {
            g_Weapons[LGT_M16].shot_accuracy = DEG_1 * 4;
            g_Weapons[LGT_M16].damage = 3;
        }
    }

    if (Gun_FireWeapon(LGT_M16, g_Lara.target, g_LaraItem, angles)) {
        g_Lara.right_arm.flash_gun = g_Weapons[LGT_M16].flash_time;
    }
}

void Gun_Rifle_FireHarpoon(void)
{
    if (g_Lara.harpoon_ammo.ammo <= 0) {
        return;
    }

    const int16_t item_num = Item_Create();
    if (item_num == NO_ITEM) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    item->object_id = O_HARPOON_BOLT;
    item->room_num = g_LaraItem->room_num;

    XYZ_32 offset = {
        .x = -2,
        .y = 373,
        .z = 77,
    };

    Lara_GetJointAbsPosition(&offset, LM_HAND_R);
    item->pos.x = offset.x;
    item->pos.y = offset.y;
    item->pos.z = offset.z;
    Item_Initialise(item_num);

    if (g_Lara.target != nullptr) {
        GAME_VECTOR lara_vec;
        Gun_FindTargetPoint(g_Lara.target, &lara_vec);
        const int32_t dx = lara_vec.pos.x - item->pos.x;
        const int32_t dz = lara_vec.pos.z - item->pos.z;
        const int32_t dy = lara_vec.pos.y - item->pos.y;
        const int32_t dxz = Math_Sqrt(SQUARE(dx) + SQUARE(dz));
        item->rot.y = Math_Atan(dz, dx);
        item->rot.x = -Math_Atan(dxz, dy);
        item->rot.z = 0;
    } else {
        item->rot.x = g_Lara.left_arm.rot.x + g_LaraItem->rot.x;
        item->rot.y = g_Lara.left_arm.rot.y + g_LaraItem->rot.y;
        item->rot.z = 0;
    }

    item->fall_speed =
        (-HARPOON_BOLT_SPEED * Math_Sin(item->rot.x)) >> W2V_SHIFT;
    item->speed = (HARPOON_BOLT_SPEED * Math_Cos(item->rot.x)) >> W2V_SHIFT;
    Item_AddActive(item_num);
    item->status = IS_ACTIVE;

    g_Lara.harpoon_ammo.ammo--;
    if (g_SaveGame.bonus_flag
        && (g_Lara.harpoon_ammo.ammo % HARPOON_RECOIL) == 0) {
        g_Lara.harpoon_ammo.ammo += HARPOON_RECOIL;
    }
    Stats_AddAmmoUsed();
}

void Gun_Rifle_FireGrenade(void)
{
    if (g_Lara.grenade_ammo.ammo <= 0) {
        return;
    }

    const int16_t item_num = Item_Create();
    if (item_num == NO_ITEM) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    item->object_id = O_GRENADE;
    item->room_num = g_LaraItem->room_num;

    XYZ_32 offset = {
        .x = -2,
        .y = 373,
        .z = 77,
    };
    Lara_GetJointAbsPosition(&offset, LM_HAND_R);
    item->pos.x = offset.x;
    item->pos.y = offset.y;
    item->pos.z = offset.z;
    Item_Initialise(item_num);

    item->rot.x = g_Lara.left_arm.rot.x + g_LaraItem->rot.x;
    item->rot.y = g_Lara.left_arm.rot.y + g_LaraItem->rot.y;
    item->rot.z = 0;
    item->speed = GRENADE_SPEED;
    item->fall_speed = 0;
    Item_AddActive(item_num);
    item->status = IS_ACTIVE;

    if (!g_SaveGame.bonus_flag) {
        g_Lara.grenade_ammo.ammo--;
    }
    Stats_AddAmmoUsed();
}

void Gun_Rifle_Draw(const LARA_GUN_TYPE weapon_type)
{
    ITEM *item;
    if (g_Lara.weapon_item != NO_ITEM) {
        item = Item_Get(g_Lara.weapon_item);
    } else {
        g_Lara.weapon_item = Item_Create();
        item = Item_Get(g_Lara.weapon_item);
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
    ITEM *const item = Item_Get(g_Lara.weapon_item);
    if (g_Lara.water_status == LWS_SURFACE) {
        item->goal_anim_state = LA_G_SURF_UNDRAW;
    } else {
        item->goal_anim_state = LA_G_UNDRAW;
    }
    M_AnimateGun(item);

    if (item->status == IS_DEACTIVATED) {
        Item_Kill(g_Lara.weapon_item);
        g_Lara.weapon_item = NO_ITEM;
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
    ITEM *const item = Item_Get(g_Lara.weapon_item);

    switch (item->current_anim_state) {
    case LA_G_AIM:
        m_M16Firing = false;
        if (m_HarpoonFired) {
            item->goal_anim_state = LA_G_RELOAD;
            m_HarpoonFired = false;
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
        if (m_HarpoonFired) {
            item->goal_anim_state = LA_G_RELOAD;
            m_HarpoonFired = false;
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
                && !m_HarpoonFired) {
                if (g_Input.action) {
                    if (g_Lara.target == nullptr || g_Lara.left_arm.lock) {
                        switch (weapon_type) {
                        case LGT_HARPOON:
                            Gun_Rifle_FireHarpoon();
                            if ((g_Lara.harpoon_ammo.ammo % HARPOON_RECOIL)
                                == 0) {
                                m_HarpoonFired = true;
                            }
                            break;

                        case LGT_GRENADE:
                            Gun_Rifle_FireGrenade();
                            break;

                        case LGT_M16:
                            Gun_Rifle_FireM16(false);
                            Sound_Effect(
                                SFX_M16_FIRE, &g_LaraItem->pos, SPM_NORMAL);
                            m_M16Firing = true;
                            break;

                        default:
                            Gun_Rifle_FireShotgun();
                            break;
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
                && !m_HarpoonFired) {
                if (g_Input.action) {
                    if (g_Lara.target == nullptr || g_Lara.left_arm.lock) {
                        if (weapon_type == LGT_HARPOON) {
                            Gun_Rifle_FireHarpoon();
                            if ((g_Lara.harpoon_ammo.ammo % HARPOON_RECOIL)
                                == 0) {
                                m_HarpoonFired = true;
                            }
                        } else {
                            Gun_Rifle_FireM16(true);
                        }
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
