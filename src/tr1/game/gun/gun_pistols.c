#include "game/gun/gun_pistols.h"

#include "game/gun.h"
#include "game/gun/gun_misc.h"
#include "game/input.h"
#include "game/sound.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/gun/vars.h>

#include <stdint.h>

void Gun_Pistols_SetArmInfo(LARA_ARM *const arm, const int32_t frame)
{
    arm->frame_num = frame;
}

void Gun_Pistols_Control(const LARA_GUN_TYPE weapon_type)
{
    WEAPON_INFO *winfo = &g_Weapons[weapon_type];

    Gun_GetNewTarget(winfo);

    if (g_InputDB.change_target && g_Config.gameplay.enable_target_change) {
        Gun_ChangeTarget(winfo);
    }

    Gun_AimWeapon(winfo, &g_Lara.left_arm);
    Gun_AimWeapon(winfo, &g_Lara.right_arm);

    if (g_Lara.left_arm.lock && !g_Lara.right_arm.lock) {
        if (g_Camera.type != CAM_LOOK) {
            g_Lara.head_rot.x = g_Lara.left_arm.rot.x / 2;
            g_Lara.head_rot.y = g_Lara.left_arm.rot.y / 2;
        }
        g_Lara.torso_rot.x = g_Lara.left_arm.rot.x / 2;
        g_Lara.torso_rot.y = g_Lara.left_arm.rot.y / 2;
    } else if (!g_Lara.left_arm.lock && g_Lara.right_arm.lock) {
        if (g_Camera.type != CAM_LOOK) {
            g_Lara.head_rot.x = g_Lara.right_arm.rot.x / 2;
            g_Lara.head_rot.y = g_Lara.right_arm.rot.y / 2;
        }
        g_Lara.torso_rot.x = g_Lara.right_arm.rot.x / 2;
        g_Lara.torso_rot.y = g_Lara.right_arm.rot.y / 2;
    } else if (g_Lara.left_arm.lock && g_Lara.right_arm.lock) {
        if (g_Camera.type != CAM_LOOK) {
            g_Lara.head_rot.x =
                (g_Lara.right_arm.rot.x + g_Lara.left_arm.rot.x) / 4;
            g_Lara.head_rot.y =
                (g_Lara.right_arm.rot.y + g_Lara.left_arm.rot.y) / 4;
        }
        g_Lara.torso_rot.x =
            (g_Lara.right_arm.rot.x + g_Lara.left_arm.rot.x) / 4;
        g_Lara.torso_rot.y =
            (g_Lara.right_arm.rot.y + g_Lara.left_arm.rot.y) / 4;
    }

    Gun_Pistols_Animate(weapon_type);

    if (g_Lara.left_arm.flash_gun || g_Lara.right_arm.flash_gun) {
        Gun_AddDynamicLight();
    }
}

void Gun_Pistols_Animate(const LARA_GUN_TYPE weapon_type)
{
    PHD_ANGLE angles[2];
    WEAPON_INFO *winfo = &g_Weapons[weapon_type];

    int16_t frame_r = g_Lara.right_arm.frame_num;
    if (g_Lara.right_arm.lock || (g_Input.action && !g_Lara.target)) {
        if (Anim_TestAbsFrameRange(frame_r, LF_G_AIM_START, LF_G_AIM_EXTEND)) {
            frame_r++;
        } else if (
            Anim_TestAbsFrameEqual(frame_r, LF_G_AIM_END) && g_Input.action) {
            angles[0] = g_Lara.right_arm.rot.y + g_LaraItem->rot.y;
            angles[1] = g_Lara.right_arm.rot.x;
            if (Gun_FireWeapon(
                    weapon_type, g_Lara.target, g_LaraItem, angles)) {
                g_Lara.right_arm.flash_gun = winfo->flash_time;
                Sound_Effect(winfo->sample_num, &g_LaraItem->pos, SPM_NORMAL);
            }
            frame_r = LF_G_RECOIL_START;
        } else if (Anim_TestAbsFrameRange(
                       frame_r, LF_G_RECOIL_START, LF_G_RECOIL_END)) {
            frame_r++;
            if (Anim_TestAbsFrameEqual(
                    frame_r, LF_G_RECOIL_START + winfo->recoil_frame)) {
                frame_r = LF_G_AIM_END;
            }
        }
    } else if (Anim_TestAbsFrameRange(
                   frame_r, LF_G_RECOIL_START, LF_G_RECOIL_END)) {
        frame_r = LF_G_AIM_END;
    } else if (Anim_TestAbsFrameRange(frame_r, LF_G_AIM_BEND, LF_G_AIM_END)) {
        frame_r--;
    }
    g_Lara.right_arm.frame_num = frame_r;

    int16_t frame_l = g_Lara.left_arm.frame_num;
    if (g_Lara.left_arm.lock || (g_Input.action && !g_Lara.target)) {
        if (Anim_TestAbsFrameRange(frame_l, LF_G_AIM_START, LF_G_AIM_EXTEND)) {
            frame_l++;
        } else if (
            Anim_TestAbsFrameEqual(frame_l, LF_G_AIM_END) && g_Input.action) {
            angles[0] = g_Lara.left_arm.rot.y + g_LaraItem->rot.y;
            angles[1] = g_Lara.left_arm.rot.x;
            if (Gun_FireWeapon(
                    weapon_type, g_Lara.target, g_LaraItem, angles)) {
                g_Lara.left_arm.flash_gun = winfo->flash_time;
                Sound_Effect(winfo->sample_num, &g_LaraItem->pos, SPM_NORMAL);
            }
            frame_l = LF_G_RECOIL_START;
        } else if (Anim_TestAbsFrameRange(
                       frame_l, LF_G_RECOIL_START, LF_G_RECOIL_END)) {
            frame_l++;
            if (Anim_TestAbsFrameEqual(
                    frame_l, LF_G_RECOIL_START + winfo->recoil_frame)) {
                frame_l = LF_G_AIM_END;
            }
        }
    } else if (Anim_TestAbsFrameRange(
                   frame_l, LF_G_RECOIL_START, LF_G_RECOIL_END)) {
        frame_l = LF_G_AIM_END;
    } else if (Anim_TestAbsFrameRange(frame_l, LF_G_AIM_BEND, LF_G_AIM_END)) {
        frame_l--;
    }
    g_Lara.left_arm.frame_num = frame_l;
}
