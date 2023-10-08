#include "game/gun/gun_rifle.h"

#include "config.h"
#include "game/gun/gun_misc.h"
#include "game/input.h"
#include "game/items.h"
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

    if (!Item_TestAbsFrameRange(ani, LF_SG_DRAW, LF_SG_RECOIL)) {
        ani = LF_SG_DRAW;
    } else if (Item_TestAbsFrameEqual(ani, LF_SG_DRAW + 10)) {
        Gun_Rifle_DrawMeshes();
        Sound_Effect(SFX_LARA_DRAW, &g_LaraItem->pos, SPM_NORMAL);
    } else if (Item_TestAbsFrameEqual(ani, LF_SG_RECOIL)) {
        Gun_Rifle_Ready();
        ani = LF_SG_AIM;
    }
    g_Lara.left_arm.frame_number = ani;
    g_Lara.right_arm.frame_number = ani;
}

void Gun_Rifle_Undraw(void)
{
    int16_t ani = ani = g_Lara.left_arm.frame_number;

    if (Item_TestAbsFrameEqual(ani, LF_SG_AIM)) {
        ani = LF_SG_UNDRAW;
    } else if (Item_TestAbsFrameRange(ani, LF_SG_AIM, LF_SG_DRAW - 1)) {
        ani++;
        if (Item_TestAbsFrameEqual(ani, LF_SG_DRAW)) {
            ani = LF_SG_UNAIM;
        }
    } else if (Item_TestAbsFrameEqual(ani, LF_SG_RECOIL)) {
        ani = LF_SG_UNAIM;
    } else if (Item_TestAbsFrameRange(ani, LF_SG_RECOIL, LF_SG_UNDRAW - 1)) {
        ani++;
        if (Item_TestAbsFrameEqual(ani, LF_SG_RECOIL + 12)) {
            ani = LF_SG_AIM;
        } else if (Item_TestAbsFrameEqual(ani, LF_SG_UNDRAW)) {
            ani = LF_SG_UNAIM;
        }
    } else if (Item_TestAbsFrameRange(ani, LF_SG_UNAIM, LF_SG_END - 1)) {
        ani++;
        if (Item_TestAbsFrameEqual(ani, LF_SG_END)) {
            ani = LF_SG_UNDRAW;
        }
    } else if (Item_TestAbsFrameRange(ani, LF_SG_UNDRAW, LF_SG_UNAIM - 1)) {
        ani++;
        if (Item_TestAbsFrameEqual(ani, LF_SG_UNDRAW + 20)) {
            Gun_Rifle_UndrawMeshes();
            Sound_Effect(SFX_LARA_DRAW, &g_LaraItem->pos, SPM_NORMAL);
        } else if (Item_TestAbsFrameEqual(ani, LF_SG_UNAIM)) {
            ani = LF_SG_AIM;
            g_Lara.gun_status = LGS_ARMLESS;
            g_Lara.target = NULL;
            g_Lara.right_arm.lock = 0;
            g_Lara.left_arm.lock = 0;
        }
    }

    g_Lara.head_x_rot = 0;
    g_Lara.head_y_rot = 0;
    g_Lara.torso_x_rot += g_Lara.torso_x_rot / -2;
    g_Lara.torso_y_rot += g_Lara.torso_y_rot / -2;
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
    g_Lara.left_arm.x_rot = 0;
    g_Lara.left_arm.y_rot = 0;
    g_Lara.left_arm.z_rot = 0;
    g_Lara.left_arm.lock = 0;
    g_Lara.right_arm.x_rot = 0;
    g_Lara.right_arm.y_rot = 0;
    g_Lara.right_arm.z_rot = 0;
    g_Lara.right_arm.lock = 0;
    g_Lara.head_x_rot = 0;
    g_Lara.head_y_rot = 0;
    g_Lara.torso_x_rot = 0;
    g_Lara.torso_y_rot = 0;
    g_Lara.target = NULL;
    g_Lara.right_arm.frame_base = g_Objects[O_SHOTGUN].frame_base;
    g_Lara.left_arm.frame_base = g_Objects[O_SHOTGUN].frame_base;
}

void Gun_Rifle_Control(LARA_GUN_TYPE weapon_type)
{
    WEAPON_INFO *winfo = &g_Weapons[LGT_SHOTGUN];

    if (g_Input.action) {
        Gun_TargetInfo(winfo);
    } else {
        g_Lara.target = NULL;
    }
    if (!g_Lara.target) {
        Gun_GetNewTarget(winfo);
    }

    Gun_AimWeapon(winfo, &g_Lara.left_arm);

    if (g_Lara.left_arm.lock) {
        g_Lara.torso_y_rot = g_Lara.left_arm.y_rot / 2;
        g_Lara.torso_x_rot = g_Lara.left_arm.x_rot / 2;
        g_Lara.head_x_rot = 0;
        g_Lara.head_y_rot = 0;
    }

    Gun_Rifle_Animate();
}

void Gun_Rifle_Animate(void)
{
    int16_t ani = g_Lara.left_arm.frame_number;
    if (g_Lara.left_arm.lock) {
        if (Item_TestAbsFrameRange(ani, LF_SG_AIM, LF_SG_DRAW - 1)) {
            ani++;
            if (Item_TestAbsFrameEqual(ani, LF_SG_DRAW)) {
                ani = LF_SG_RECOIL;
            }
        } else if (Item_TestAbsFrameEqual(ani, LF_SG_RECOIL)) {
            if (g_Input.action) {
                Gun_Rifle_Fire();
                ani++;
            }
        } else if (Item_TestAbsFrameRange(
                       ani, LF_SG_RECOIL + 1, LF_SG_UNDRAW - 1)) {
            ani++;
            if (Item_TestAbsFrameEqual(ani, LF_SG_UNDRAW)) {
                ani = LF_SG_RECOIL;
            } else if (Item_TestAbsFrameEqual(ani, LF_SG_RECOIL + 10)) {
                Sound_Effect(SFX_LARA_RELOAD, &g_LaraItem->pos, SPM_NORMAL);
            }
        } else if (Item_TestAbsFrameRange(ani, LF_SG_UNAIM, LF_SG_END - 1)) {
            ani++;
            if (Item_TestAbsFrameEqual(ani, LF_SG_END)) {
                ani = LF_SG_AIM;
            }
        }
    } else if (g_Config.fix_shotgun_targeting && g_Lara.target) {
        if (Item_TestAbsFrameEqual(ani, LF_SG_RECOIL)) {
            ani = LF_SG_UNAIM;
            ani++;
        } else if (Item_TestAbsFrameRange(
                       ani, LF_SG_RECOIL + 1, LF_SG_UNDRAW - 1)) {
            ani++;
            if (Item_TestAbsFrameEqual(ani, LF_SG_RECOIL + 10)) {
                Sound_Effect(SFX_LARA_RELOAD, &g_LaraItem->pos, SPM_NORMAL);
            } else if (Item_TestAbsFrameEqual(ani, LF_SG_RECOIL + 16)) {
                ani = LF_SG_AIM;
            } else if (Item_TestAbsFrameEqual(ani, LF_SG_UNDRAW)) {
                ani = LF_SG_UNAIM;
            }
        } else if (Item_TestAbsFrameRange(ani, LF_SG_UNAIM, LF_SG_END - 1)) {
            ani++;
            if (Item_TestAbsFrameEqual(ani, LF_SG_END)) {
                ani = LF_SG_AIM;
            }
        } else if (Item_TestAbsFrameRange(ani, LF_SG_AIM + 1, LF_SG_DRAW)) {
            ani--;
        }
    } else {
        if (g_Input.action && Item_TestAbsFrameEqual(ani, LF_SG_AIM)) {
            ani++;
        } else if (Item_TestAbsFrameRange(ani, LF_SG_AIM + 1, LF_SG_DRAW - 1)) {
            ani++;
            if (Item_TestAbsFrameEqual(ani, LF_SG_DRAW)) {
                ani = g_Input.action ? LF_SG_RECOIL : LF_SG_UNAIM;
            }
        } else if (Item_TestAbsFrameEqual(ani, LF_SG_RECOIL)) {
            if (g_Input.action) {
                Gun_Rifle_Fire();
                ani++;
            } else {
                ani = LF_SG_UNAIM;
            }
        } else if (Item_TestAbsFrameRange(
                       ani, LF_SG_RECOIL + 1, LF_SG_UNDRAW - 1)) {
            ani++;
            if (g_Config.fix_shotgun_targeting) {
                if (Item_TestAbsFrameEqual(ani, LF_SG_RECOIL + 10)) {
                    Sound_Effect(SFX_LARA_RELOAD, &g_LaraItem->pos, SPM_NORMAL);
                } else if (Item_TestAbsFrameEqual(ani, LF_SG_UNDRAW)) {
                    ani = g_Input.action ? LF_SG_RECOIL : LF_SG_UNAIM;
                }
            } else {
                if (Item_TestAbsFrameEqual(ani, LF_SG_RECOIL + 12 + 1)) {
                    ani = LF_SG_AIM;
                } else if (Item_TestAbsFrameEqual(ani, LF_SG_UNDRAW)) {
                    ani = LF_SG_UNAIM;
                } else if (Item_TestAbsFrameEqual(ani, LF_SG_RECOIL + 10)) {
                    Sound_Effect(SFX_LARA_RELOAD, &g_LaraItem->pos, SPM_NORMAL);
                }
            }
        } else if (Item_TestAbsFrameRange(ani, LF_SG_UNAIM, LF_SG_END - 1)) {
            ani++;
            if (Item_TestAbsFrameEqual(ani, LF_SG_END)) {
                ani = LF_SG_AIM;
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

    angles[0] = g_Lara.left_arm.y_rot + g_LaraItem->pos.y_rot;
    angles[1] = g_Lara.left_arm.x_rot;

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
