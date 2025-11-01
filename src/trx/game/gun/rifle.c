#include <trx/game/gun/rifle.h>

#include <trx/config.h>
#include <trx/game/camera.h>
#include <trx/game/game.h>
#include <trx/game/gun/common.h>
#include <trx/game/gun/const.h>
#include <trx/game/gun/control.h>
#include <trx/game/gun/misc.h>
#include <trx/game/gun/vars.h>
#include <trx/game/lara.h>
#include <trx/game/random.h>
#include <trx/game/sound.h>
#include <trx/game/stats.h>

#define M_EQUIP_ANIM 1
#define M_DRAW_FRAME 10
#define M_UNDRAW_FRAME 21

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
} M_ANIM;

static bool m_M16Firing = false;
static bool m_ReloadHarpoon = false;

static M_ANIM M_GetReadyAnim(const LARA_GUN_TYPE weapon_type)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    switch (weapon_type) {
    case LGT_HARPOON:
        return lara->water_status == LWS_UNDERWATER ? LA_G_UAIM : LA_G_AIM;
    case LGT_GRENADE:
        return LA_G_DRAW;
    default:
        return LA_G_AIM;
    }
}

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

static void M_Ready(const LARA_GUN_TYPE weapon_type)
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
        m_ReloadHarpoon = true;
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

static void M_Fire(const LARA_GUN_TYPE weapon_type, const bool running)
{
    switch (weapon_type) {
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
    default:
        if (!running) {
            M_FireGeneric(weapon_type);
        }
        break;
    }
}

static void M_Animate(const LARA_GUN_TYPE weapon_type)
{
    const ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara = Lara_GetLaraInfo();

    const bool running = weapon_type == LGT_M16 && lara_item->speed != 0;
    ITEM *const item = Item_Get(lara->gun_item_num);

    switch (item->current_anim_state) {
    case LA_G_AIM:
        m_M16Firing = false;
        if (m_ReloadHarpoon) {
            item->goal_anim_state = LA_G_RELOAD;
            m_ReloadHarpoon = false;
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
        if (m_ReloadHarpoon) {
            item->goal_anim_state = LA_G_RELOAD;
            m_ReloadHarpoon = false;
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
                && !m_ReloadHarpoon) {
                if (g_Input.action) {
                    if (lara->target == nullptr || lara->left_arm.lock) {
                        M_Fire(weapon_type, false);
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
                && !m_ReloadHarpoon) {
                if (g_Input.action) {
                    if (lara->target == nullptr || lara->left_arm.lock) {
                        M_Fire(weapon_type, true);
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

void Gun_Rifle_Control(const LARA_GUN_TYPE weapon_type)
{
    const WEAPON_INFO *const weapon = &g_Weapons[weapon_type];
    LARA_INFO *const lara = Lara_GetLaraInfo();

    Gun_GetNewTarget(weapon);
    if (g_InputDB.change_target && g_Config.gameplay.enable_target_change) {
        Gun_ChangeTarget(weapon);
    }

    Gun_AimWeapon(weapon, &lara->left_arm);

    if (lara->left_arm.lock) {
        lara->torso_rot.x = lara->left_arm.rot.x;
        lara->torso_rot.y = lara->left_arm.rot.y;
        if (g_Config.gameplay.look_mode != LOOK_MODE_UNRESTRICTED
            || g_Camera.type != CAM_LOOK) {
            lara->head_rot.x = 0;
            lara->head_rot.y = 0;
        }
    }

    M_Animate(weapon_type);

    if (lara->right_arm.flash_gun
        && (weapon_type == LGT_SHOTGUN || weapon_type == LGT_M16)) {
        Gun_AddDynamicLight();
    }
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
            Item_SwitchToAnim(item, M_EQUIP_ANIM, 0);
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
        M_Ready(weapon_type);
    } else if (Item_TestFrameEqual(item, M_DRAW_FRAME)) {
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
        && Item_TestFrameEqual(item, M_UNDRAW_FRAME)) {
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

void Gun_Rifle_EnsureReady(const LARA_GUN_TYPE weapon_type)
{
    Gun_Rifle_Draw(weapon_type);

    const LARA_INFO *const lara = Lara_GetLaraInfo();
    const ITEM *const item = Item_Get(lara->gun_item_num);
    const int16_t goal_anim = M_GetReadyAnim(weapon_type);

    do {
        Gun_Rifle_Draw(weapon_type);
    } while (Item_GetRelativeAnim(item) != goal_anim);
}

void Gun_Rifle_LoadLegacy(const bool has_rifle)
{
    // Applies to legacy TR1 saves where there was no concept of the back gun.
    // Restore Lara's torso to normal and either snaps to ready or undrawn
    // state.
    LARA_INFO *const lara = Lara_GetLaraInfo();
    Lara_Mesh_SwapSingle(LM_TORSO, O_LARA);

    if (lara->gun_type == LGT_SHOTGUN) {
        Gun_Rifle_UndrawMeshes(LGT_SHOTGUN);
        if (lara->gun_status == LGS_DRAW || lara->gun_status == LGS_READY) {
            Gun_Rifle_EnsureReady(LGT_SHOTGUN);
        } else if (lara->gun_status == LGS_UNDRAW) {
            lara->gun_status = LGS_ARMLESS;
        }
    } else if (has_rifle) {
        lara->back_gun_obj_id = O_LARA_SHOTGUN;
    }
}
