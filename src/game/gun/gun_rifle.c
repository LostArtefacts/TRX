#include "game/gun/gun_rifle.h"

#include "config.h"
#include "game/anim.h"
#include "game/gun/gun_misc.h"
#include "game/input.h"
#include "game/random.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/vars.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void Gun_Rifle_Draw(void)
{
    int16_t ani = g_Lara.left_arm.frame_number;
    ani++;

    if (!Anim_TestAbsFrameRange(ani, LF_SG_DRAW_START, LF_SG_RECOIL_START)) {
        ani = LF_SG_DRAW_START;
    } else if (Anim_TestAbsFrameEqual(ani, LF_SG_DRAW_SFX)) {
        Gun_Rifle_DrawMeshes();
        Sound_Effect(SFX_LARA_DRAW, &g_LaraItem->pos, SPM_NORMAL);
    } else if (Anim_TestAbsFrameEqual(ani, LF_SG_RECOIL_START)) {
        Gun_Rifle_Ready();
        ani = LF_SG_AIM_START;
    }
    g_Lara.left_arm.frame_number = ani;
    g_Lara.right_arm.frame_number = ani;
}

void Gun_Rifle_Undraw(void)
{
    int16_t ani = ani = g_Lara.left_arm.frame_number;

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
            Gun_Rifle_UndrawMeshes();
            Sound_Effect(SFX_LARA_DRAW, &g_LaraItem->pos, SPM_NORMAL);
        } else if (Anim_TestAbsFrameEqual(ani, LF_SG_UNAIM_START)) {
            ani = LF_SG_AIM_START;
            g_Lara.gun_status = LGS_ARMLESS;
            g_Lara.target = NULL;
            g_Lara.right_arm.lock = 0;
            g_Lara.left_arm.lock = 0;
        }
    }

    g_Lara.head_rot.x = 0;
    g_Lara.head_rot.y = 0;
    g_Lara.torso_rot.x += g_Lara.torso_rot.x / -2;
    g_Lara.torso_rot.y += g_Lara.torso_rot.y / -2;
    g_Lara.right_arm.frame_number = ani;
    g_Lara.left_arm.frame_number = ani;
}

void Gun_Rifle_DrawMeshes(void)
{
    g_Lara.mesh_ptrs[LM_HAND_L] =
        g_Meshes[g_Objects[O_SHOTGUN].mesh_index + LM_HAND_L];
    g_Lara.mesh_ptrs[LM_HAND_R] =
        g_Meshes[g_Objects[O_SHOTGUN].mesh_index + LM_HAND_R];
    g_Lara.mesh_ptrs[LM_TORSO] =
        g_Meshes[g_Objects[O_LARA].mesh_index + LM_TORSO];
}

void Gun_Rifle_UndrawMeshes(void)
{
    g_Lara.mesh_ptrs[LM_HAND_L] =
        g_Meshes[g_Objects[O_LARA].mesh_index + LM_HAND_L];
    g_Lara.mesh_ptrs[LM_HAND_R] =
        g_Meshes[g_Objects[O_LARA].mesh_index + LM_HAND_R];
    g_Lara.mesh_ptrs[LM_TORSO] =
        g_Meshes[g_Objects[O_SHOTGUN].mesh_index + LM_TORSO];
}

void Gun_Rifle_Ready(void)
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
    g_Lara.right_arm.frame_base_new = g_Objects[O_SHOTGUN].frame_base_new;
    g_Lara.left_arm.frame_base_new = g_Objects[O_SHOTGUN].frame_base_new;
}

void Gun_Rifle_Control(LARA_GUN_TYPE weapon_type)
{
    WEAPON_INFO *winfo = &g_Weapons[LGT_SHOTGUN];

    Gun_GetNewTarget(winfo);

    if (g_InputDB.change_target && g_Config.enable_target_change) {
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

    Gun_Rifle_Animate();
}

void Gun_Rifle_Animate(void)
{
    int16_t ani = g_Lara.left_arm.frame_number;
    if (g_Lara.left_arm.lock) {
        if (Anim_TestAbsFrameRange(ani, LF_SG_AIM_START, LF_SG_AIM_END)) {
            ani++;
            if (Anim_TestAbsFrameEqual(ani, LF_SG_DRAW_START)) {
                ani = LF_SG_RECOIL_START;
            }
        } else if (Anim_TestAbsFrameEqual(ani, LF_SG_RECOIL_START)) {
            if (g_Input.action) {
                Gun_Rifle_Fire();
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
    } else if (g_Config.fix_shotgun_targeting && g_Lara.target) {
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
                Gun_Rifle_Fire();
                ani++;
            } else {
                ani = LF_SG_UNAIM_START;
            }
        } else if (Anim_TestAbsFrameRange(
                       ani, LF_SG_RECOILING, LF_SG_RECOIL_END)) {
            ani++;
            if (g_Config.fix_shotgun_targeting) {
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
    g_Lara.right_arm.frame_number = ani;
    g_Lara.left_arm.frame_number = ani;
}

void Gun_Rifle_Fire(void)
{
    bool fired = false;
    PHD_ANGLE angles[2];
    PHD_ANGLE dangles[2];

    angles[0] = g_Lara.left_arm.rot.y + g_LaraItem->rot.y;
    angles[1] = g_Lara.left_arm.rot.x;

    for (int i = 0; i < SHOTGUN_AMMO_CLIP; i++) {
        dangles[0] = angles[0]
            + (int)((Random_GetControl() - 16384) * PELLET_SCATTER) / 65536;
        dangles[1] = angles[1]
            + (int)((Random_GetControl() - 16384) * PELLET_SCATTER) / 65536;
        if (Gun_FireWeapon(LGT_SHOTGUN, g_Lara.target, g_LaraItem, dangles)) {
            fired = true;
        }
    }

    if (!fired) {
        return;
    }

    if (g_Config.enable_shotgun_flash) {
        g_Lara.right_arm.flash_gun = g_Weapons[LGT_SHOTGUN].flash_time;
    }
    Sound_Effect(
        g_Weapons[LGT_SHOTGUN].sample_num, &g_LaraItem->pos, SPM_NORMAL);
}
