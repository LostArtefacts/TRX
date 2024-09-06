#include "game/gun/gun_pistols.h"

#include "config.h"
#include "game/anim.h"
#include "game/gun.h"
#include "game/gun/gun_misc.h"
#include "game/input.h"
#include "game/sound.h"
#include "global/vars.h"

#include <stddef.h>
#include <stdint.h>

void Gun_Pistols_Draw(const LARA_GUN_TYPE weapon_type)
{
    int16_t ani = g_Lara.left_arm.frame_num;
    ani++;

    if (!Anim_TestAbsFrameRange(ani, LF_G_UNDRAW_START, LF_G_DRAW_END)) {
        ani = LF_G_UNDRAW_START;
    } else if (Anim_TestAbsFrameEqual(ani, LF_G_DRAW_START)) {
        Gun_Pistols_DrawMeshes(weapon_type);
        Sound_Effect(SFX_LARA_DRAW, &g_LaraItem->pos, SPM_NORMAL);
    } else if (Anim_TestAbsFrameEqual(ani, LF_G_DRAW_END)) {
        Gun_Pistols_Ready(weapon_type);
        ani = LF_G_AIM_START;
    }

    g_Lara.left_arm.frame_num = ani;
    g_Lara.right_arm.frame_num = ani;
}

void Gun_Pistols_Undraw(const LARA_GUN_TYPE weapon_type)
{
    int16_t anil = g_Lara.left_arm.frame_num;
    if (Anim_TestAbsFrameRange(anil, LF_G_RECOIL_START, LF_G_RECOIL_END)) {
        anil = LF_G_AIM_END;
    } else if (Anim_TestAbsFrameRange(anil, LF_G_AIM_BEND, LF_G_AIM_END)) {
        g_Lara.left_arm.rot.x -= g_Lara.left_arm.rot.x / anil;
        g_Lara.left_arm.rot.y -= g_Lara.left_arm.rot.y / anil;
        anil--;
    } else if (Anim_TestAbsFrameEqual(anil, LF_G_AIM_START)) {
        g_Lara.left_arm.rot.x = 0;
        g_Lara.left_arm.rot.y = 0;
        g_Lara.left_arm.rot.z = 0;
        anil = LF_G_DRAW_END;
    } else if (Anim_TestAbsFrameEqual(anil, LF_G_DRAW_START)) {
        Gun_Pistols_UndrawMeshLeft(weapon_type);
        anil--;
    } else if (Anim_TestAbsFrameRange(anil, LF_G_UNDRAW_BEND, LF_G_DRAW_END)) {
        anil--;
    }
    g_Lara.left_arm.frame_num = anil;

    int16_t anir = g_Lara.right_arm.frame_num;
    if (Anim_TestAbsFrameRange(anir, LF_G_RECOIL_START, LF_G_RECOIL_END)) {
        anir = LF_G_AIM_END;
    } else if (Anim_TestAbsFrameRange(anir, LF_G_AIM_BEND, LF_G_AIM_END)) {
        g_Lara.right_arm.rot.x -= g_Lara.right_arm.rot.x / anir;
        g_Lara.right_arm.rot.y -= g_Lara.right_arm.rot.y / anir;
        anir--;
    } else if (Anim_TestAbsFrameEqual(anir, LF_G_AIM_START)) {
        g_Lara.right_arm.rot.x = 0;
        g_Lara.right_arm.rot.y = 0;
        g_Lara.right_arm.rot.z = 0;
        anir = LF_G_DRAW_END;
    } else if (Anim_TestAbsFrameEqual(anir, LF_G_DRAW_START)) {
        Gun_Pistols_UndrawMeshRight(weapon_type);
        anir--;
    } else if (Anim_TestAbsFrameRange(anir, LF_G_UNDRAW_BEND, LF_G_DRAW_END)) {
        anir--;
    }
    g_Lara.right_arm.frame_num = anir;

    if (Anim_TestAbsFrameEqual(anil, LF_G_UNDRAW_START)
        && Anim_TestAbsFrameEqual(anir, LF_G_UNDRAW_START)) {
        g_Lara.left_arm.lock = 0;
        g_Lara.right_arm.lock = 0;
        g_Lara.left_arm.frame_num = LF_G_AIM_START;
        g_Lara.right_arm.frame_num = LF_G_AIM_START;
        g_Lara.gun_status = LGS_ARMLESS;
        g_Lara.target = NULL;
    }

    g_Lara.head_rot.x = (g_Lara.right_arm.rot.x + g_Lara.left_arm.rot.x) / 4;
    g_Lara.head_rot.y = (g_Lara.right_arm.rot.y + g_Lara.left_arm.rot.y) / 4;
    g_Lara.torso_rot.x = (g_Lara.right_arm.rot.x + g_Lara.left_arm.rot.x) / 4;
    g_Lara.torso_rot.y = (g_Lara.right_arm.rot.y + g_Lara.left_arm.rot.y) / 4;
}

void Gun_Pistols_Ready(const LARA_GUN_TYPE weapon_type)
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
    g_Lara.head_rot.x = 0;
    g_Lara.head_rot.y = 0;
    g_Lara.torso_rot.x = 0;
    g_Lara.torso_rot.y = 0;
    g_Lara.target = NULL;
    g_Lara.right_arm.frame_base = g_Objects[O_PISTOL_ANIM].frame_base;
    g_Lara.left_arm.frame_base = g_Objects[O_PISTOL_ANIM].frame_base;
}

void Gun_Pistols_DrawMeshes(const LARA_GUN_TYPE weapon_type)
{
    Gun_SetLaraHandLMesh(weapon_type);
    Gun_SetLaraHandRMesh(weapon_type);
    Gun_SetLaraHolsterLMesh(LGT_UNARMED);
    Gun_SetLaraHolsterRMesh(LGT_UNARMED);
}

void Gun_Pistols_UndrawMeshLeft(const LARA_GUN_TYPE weapon_type)
{
    Gun_SetLaraHandLMesh(LGT_UNARMED);
    Gun_SetLaraHolsterLMesh(weapon_type);
    Sound_Effect(SFX_LARA_HOLSTER, &g_LaraItem->pos, SPM_NORMAL);
}

void Gun_Pistols_UndrawMeshRight(const LARA_GUN_TYPE weapon_type)
{
    Gun_SetLaraHandRMesh(LGT_UNARMED);
    Gun_SetLaraHolsterRMesh(weapon_type);
    Sound_Effect(SFX_LARA_HOLSTER, &g_LaraItem->pos, SPM_NORMAL);
}

void Gun_Pistols_Control(const LARA_GUN_TYPE weapon_type)
{
    WEAPON_INFO *winfo = &g_Weapons[weapon_type];

    Gun_GetNewTarget(winfo);

    if (g_InputDB.change_target && g_Config.enable_target_change) {
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
}

void Gun_Pistols_Animate(const LARA_GUN_TYPE weapon_type)
{
    PHD_ANGLE angles[2];
    WEAPON_INFO *winfo = &g_Weapons[weapon_type];

    int16_t anir = g_Lara.right_arm.frame_num;
    if (g_Lara.right_arm.lock || (g_Input.action && !g_Lara.target)) {
        if (Anim_TestAbsFrameRange(anir, LF_G_AIM_START, LF_G_AIM_EXTEND)) {
            anir++;
        } else if (
            Anim_TestAbsFrameEqual(anir, LF_G_AIM_END) && g_Input.action) {
            angles[0] = g_Lara.right_arm.rot.y + g_LaraItem->rot.y;
            angles[1] = g_Lara.right_arm.rot.x;
            if (Gun_FireWeapon(
                    weapon_type, g_Lara.target, g_LaraItem, angles)) {
                g_Lara.right_arm.flash_gun = winfo->flash_time;
                Sound_Effect(winfo->sample_num, &g_LaraItem->pos, SPM_NORMAL);
            }
            anir = LF_G_RECOIL_START;
        } else if (Anim_TestAbsFrameRange(
                       anir, LF_G_RECOIL_START, LF_G_RECOIL_END)) {
            anir++;
            if (Anim_TestAbsFrameEqual(
                    anir, LF_G_RECOIL_START + winfo->recoil_frame)) {
                anir = LF_G_AIM_END;
            }
        }
    } else if (Anim_TestAbsFrameRange(
                   anir, LF_G_RECOIL_START, LF_G_RECOIL_END)) {
        anir = LF_G_AIM_END;
    } else if (Anim_TestAbsFrameRange(anir, LF_G_AIM_BEND, LF_G_AIM_END)) {
        anir--;
    }
    g_Lara.right_arm.frame_num = anir;

    int16_t anil = g_Lara.left_arm.frame_num;
    if (g_Lara.left_arm.lock || (g_Input.action && !g_Lara.target)) {
        if (Anim_TestAbsFrameRange(anil, LF_G_AIM_START, LF_G_AIM_EXTEND)) {
            anil++;
        } else if (
            Anim_TestAbsFrameEqual(anil, LF_G_AIM_END) && g_Input.action) {
            angles[0] = g_Lara.left_arm.rot.y + g_LaraItem->rot.y;
            angles[1] = g_Lara.left_arm.rot.x;
            if (Gun_FireWeapon(
                    weapon_type, g_Lara.target, g_LaraItem, angles)) {
                g_Lara.left_arm.flash_gun = winfo->flash_time;
                Sound_Effect(winfo->sample_num, &g_LaraItem->pos, SPM_NORMAL);
            }
            anil = LF_G_RECOIL_START;
        } else if (Anim_TestAbsFrameRange(
                       anil, LF_G_RECOIL_START, LF_G_RECOIL_END)) {
            anil++;
            if (Anim_TestAbsFrameEqual(
                    anil, LF_G_RECOIL_START + winfo->recoil_frame)) {
                anil = LF_G_AIM_END;
            }
        }
    } else if (Anim_TestAbsFrameRange(
                   anil, LF_G_RECOIL_START, LF_G_RECOIL_END)) {
        anil = LF_G_AIM_END;
    } else if (Anim_TestAbsFrameRange(anil, LF_G_AIM_BEND, LF_G_AIM_END)) {
        anil--;
    }
    g_Lara.left_arm.frame_num = anil;
}
