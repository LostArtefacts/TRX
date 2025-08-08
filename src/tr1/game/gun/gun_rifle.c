#include "game/gun/gun_rifle.h"

#include "game/gun.h"
#include "game/gun/gun_misc.h"
#include "game/random.h"
#include "game/sound.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/gun/vars.h>
#include <libtrx/game/input.h>

#include <stdint.h>

void Gun_Rifle_Draw(const LARA_GUN_TYPE weapon_type)
{
    int16_t ani = g_Lara.left_arm.frame_num;
    ani++;

    if (!Anim_TestAbsFrameRange(ani, LF_SG_DRAW_START, LF_SG_RECOIL_START)) {
        ani = LF_SG_DRAW_START;
    } else if (Anim_TestAbsFrameEqual(ani, LF_SG_DRAW_SFX)) {
        Gun_Rifle_DrawMeshes(weapon_type);
        Sound_Effect(SFX_LARA_DRAW, &g_LaraItem->pos, SPM_NORMAL);
    } else if (Anim_TestAbsFrameEqual(ani, LF_SG_RECOIL_START)) {
        Gun_Rifle_Ready(weapon_type);
        ani = LF_SG_AIM_START;
    }
    g_Lara.left_arm.frame_num = ani;
    g_Lara.right_arm.frame_num = ani;
}

void Gun_Rifle_Undraw(const LARA_GUN_TYPE weapon_type)
{
    int16_t ani = ani = g_Lara.left_arm.frame_num;

    if (Anim_TestAbsFrameEqual(ani, LF_SG_AIM_START)) {
        ani = LF_SG_UNDRAW_START;
    } else if (Anim_TestAbsFrameRange(ani, LF_SG_AIM_START, LF_SG_AIM_END)) {
        ani++;
        if (Anim_TestAbsFrameEqual(ani, LF_SG_DRAW_START)) {
            ani = LF_SG_UNAIM_START;
        }
    } else if (Anim_TestAbsFrameEqual(ani, LF_SG_RECOIL_START)) {
        ani = LF_SG_UNAIM_START;
    } else if (Anim_TestAbsFrameRange(
                   ani, LF_SG_RECOIL_START, LF_SG_RECOIL_END)) {
        ani++;
        if (Anim_TestAbsFrameEqual(ani, LF_SG_RECOIL_UNDRAW_RESET)) {
            ani = LF_SG_AIM_START;
        } else if (Anim_TestAbsFrameEqual(ani, LF_SG_UNDRAW_START)) {
            ani = LF_SG_UNAIM_START;
        }
    } else if (Anim_TestAbsFrameRange(
                   ani, LF_SG_UNAIM_START, LF_SG_UNAIM_RAISE)) {
        ani++;
        if (Anim_TestAbsFrameEqual(ani, LF_SG_UNAIM_END)) {
            ani = LF_SG_UNDRAW_START;
        }
    } else if (Anim_TestAbsFrameRange(
                   ani, LF_SG_UNDRAW_START, LF_SG_UNDRAW_END)) {
        ani++;
        if (Anim_TestAbsFrameEqual(ani, LF_SG_UNDRAW_SFX)) {
            Gun_Rifle_UndrawMeshes(weapon_type);
            Sound_Effect(SFX_LARA_DRAW, &g_LaraItem->pos, SPM_NORMAL);
        } else if (Anim_TestAbsFrameEqual(ani, LF_SG_UNAIM_START)) {
            ani = LF_SG_AIM_START;
            g_Lara.gun_status = LGS_ARMLESS;
            g_Lara.target = nullptr;
            g_Lara.right_arm.lock = 0;
            g_Lara.left_arm.lock = 0;
        }
    }

    if (!g_Input.look || g_Config.gameplay.look_mode == LOOK_MODE_RESTRICTED) {
        g_Lara.head_rot.x = 0;
        g_Lara.head_rot.y = 0;
        g_Lara.torso_rot.x += g_Lara.torso_rot.x / -2;
        g_Lara.torso_rot.y += g_Lara.torso_rot.y / -2;
    }
    g_Lara.right_arm.frame_num = ani;
    g_Lara.left_arm.frame_num = ani;
}

void Gun_Rifle_Ready(const LARA_GUN_TYPE weapon_type)
{
    g_Lara.gun_status = LGS_READY;
    g_Lara.left_arm.rot.x = 0;
    g_Lara.left_arm.rot.y = 0;
    g_Lara.left_arm.rot.z = 0;
    g_Lara.left_arm.lock = 0;
    g_Lara.right_arm.rot.x = 0;
    g_Lara.right_arm.rot.y = 0;
    g_Lara.right_arm.rot.z = 0;
    g_Lara.right_arm.lock = 0;
    g_Lara.target = nullptr;

    if (g_Config.gameplay.look_mode == LOOK_MODE_RESTRICTED) {
        g_Lara.head_rot.x = 0;
        g_Lara.head_rot.y = 0;
        g_Lara.torso_rot.x = 0;
        g_Lara.torso_rot.y = 0;
    }

    const OBJECT *const obj = Object_Get(O_LARA_SHOTGUN);
    g_Lara.right_arm.frame_base = obj->frame_base;
    g_Lara.left_arm.frame_base = obj->frame_base;
}

void Gun_Rifle_Control(const LARA_GUN_TYPE weapon_type)
{
    WEAPON_INFO *winfo = &g_Weapons[weapon_type];

    Gun_GetNewTarget(winfo);

    if (g_InputDB.change_target && g_Config.gameplay.enable_target_change) {
        Gun_ChangeTarget(winfo);
    }

    Gun_AimWeapon(winfo, &g_Lara.left_arm);

    if (g_Lara.left_arm.lock) {
        g_Lara.torso_rot.y = g_Lara.left_arm.rot.y / 2;
        g_Lara.torso_rot.x = g_Lara.left_arm.rot.x / 2;
        if (g_Camera.type != CAM_LOOK) {
            g_Lara.head_rot.x = 0;
            g_Lara.head_rot.y = 0;
        }
    }

    Gun_Rifle_Animate(weapon_type);

    if (g_Lara.right_arm.flash_gun) {
        Gun_AddDynamicLight();
    }
}

void Gun_Rifle_Animate(const LARA_GUN_TYPE weapon_type)
{
    int16_t ani = g_Lara.left_arm.frame_num;
    if (g_Lara.left_arm.lock) {
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
                Sound_Effect(SFX_LARA_RELOAD, &g_LaraItem->pos, SPM_NORMAL);
            }
        } else if (Anim_TestAbsFrameRange(
                       ani, LF_SG_UNAIM_START, LF_SG_UNAIM_RAISE)) {
            ani++;
            if (Anim_TestAbsFrameEqual(ani, LF_SG_UNAIM_END)) {
                ani = LF_SG_AIM_START;
            }
        }
    } else if (g_Config.gameplay.fix_shotgun_targeting && g_Lara.target) {
        if (Anim_TestAbsFrameEqual(ani, LF_SG_RECOIL_START)) {
            ani = LF_SG_UNAIM_START;
            ani++;
        } else if (Anim_TestAbsFrameRange(
                       ani, LF_SG_RECOILING, LF_SG_RECOIL_END)) {
            ani++;
            if (Anim_TestAbsFrameEqual(ani, LF_SG_RECOIL_SFX)) {
                Sound_Effect(SFX_LARA_RELOAD, &g_LaraItem->pos, SPM_NORMAL);
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
                    Sound_Effect(SFX_LARA_RELOAD, &g_LaraItem->pos, SPM_NORMAL);
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
                    Sound_Effect(SFX_LARA_RELOAD, &g_LaraItem->pos, SPM_NORMAL);
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
    g_Lara.right_arm.frame_num = ani;
    g_Lara.left_arm.frame_num = ani;
}
