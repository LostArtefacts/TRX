#include "game/lara.h"

#include "config.h"
#include "game/random.h"
#include "game/input.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <stddef.h>
#include <stdint.h>

void DrawShotgun()
{
    int16_t ani = g_Lara.left_arm.frame_number;
    ani++;

    if (ani < AF_SG_DRAW || ani > AF_SG_RECOIL) {
        ani = AF_SG_DRAW;
    } else if (ani == AF_SG_DRAW + 10) {
        DrawShotgunMeshes();
        Sound_Effect(SFX_LARA_DRAW, &g_LaraItem->pos, SPM_NORMAL);
    } else if (ani == AF_SG_RECOIL) {
        ReadyShotgun();
        ani = AF_SG_AIM;
    }
    g_Lara.left_arm.frame_number = ani;
    g_Lara.right_arm.frame_number = ani;
}

void UndrawShotgun()
{
    int16_t ani = ani = g_Lara.left_arm.frame_number;

    if (ani == AF_SG_AIM) {
        ani = AF_SG_UNDRAW;
    } else if (ani >= AF_SG_AIM && ani < AF_SG_DRAW) {
        ani++;
        if (ani == AF_SG_DRAW) {
            ani = AF_SG_UNAIM;
        }
    } else if (ani == AF_SG_RECOIL) {
        ani = AF_SG_UNAIM;
    } else if (ani >= AF_SG_RECOIL && ani < AF_SG_UNDRAW) {
        ani++;
        if (ani == AF_SG_RECOIL + 12) {
            ani = AF_SG_AIM;
        } else if (ani == AF_SG_UNDRAW) {
            ani = AF_SG_UNAIM;
        }
    } else if (ani >= AF_SG_UNAIM && ani < AF_SG_END) {
        ani++;
        if (ani == AF_SG_END) {
            ani = AF_SG_UNDRAW;
        }
    } else if (ani >= AF_SG_UNDRAW && ani < AF_SG_UNAIM) {
        ani++;
        if (ani == AF_SG_UNDRAW + 20) {
            UndrawShotgunMeshes();
            Sound_Effect(SFX_LARA_DRAW, &g_LaraItem->pos, SPM_NORMAL);
        } else if (ani == AF_SG_UNAIM) {
            ani = AF_SG_AIM;
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

void DrawShotgunMeshes()
{
    g_Lara.mesh_ptrs[LM_HAND_L] =
        g_Meshes[g_Objects[O_SHOTGUN].mesh_index + LM_HAND_L];
    g_Lara.mesh_ptrs[LM_HAND_R] =
        g_Meshes[g_Objects[O_SHOTGUN].mesh_index + LM_HAND_R];
    g_Lara.mesh_ptrs[LM_TORSO] =
        g_Meshes[g_Objects[O_LARA].mesh_index + LM_TORSO];
}

void UndrawShotgunMeshes()
{
    g_Lara.mesh_ptrs[LM_HAND_L] =
        g_Meshes[g_Objects[O_LARA].mesh_index + LM_HAND_L];
    g_Lara.mesh_ptrs[LM_HAND_R] =
        g_Meshes[g_Objects[O_LARA].mesh_index + LM_HAND_R];
    g_Lara.mesh_ptrs[LM_TORSO] =
        g_Meshes[g_Objects[O_SHOTGUN].mesh_index + LM_TORSO];
}

void ReadyShotgun()
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

void RifleHandler(int32_t weapon_type)
{
    WEAPON_INFO *winfo = &g_Weapons[LGT_SHOTGUN];

    if (g_Input.action) {
        LaraTargetInfo(winfo);
    } else {
        g_Lara.target = NULL;
    }
    if (!g_Lara.target) {
        LaraGetNewTarget(winfo);
    }

    AimWeapon(winfo, &g_Lara.left_arm);

    if (g_Lara.left_arm.lock) {
        g_Lara.torso_y_rot = g_Lara.left_arm.y_rot / 2;
        g_Lara.torso_x_rot = g_Lara.left_arm.x_rot / 2;
        g_Lara.head_x_rot = 0;
        g_Lara.head_y_rot = 0;
    }

    AnimateShotgun();
}

void AnimateShotgun()
{
    int16_t ani = g_Lara.left_arm.frame_number;
    if (g_Lara.left_arm.lock) {
        if (ani >= AF_SG_AIM && ani < AF_SG_DRAW) {
            ani++;
            if (ani == AF_SG_DRAW) {
                ani = AF_SG_RECOIL;
            }
        } else if (ani == AF_SG_RECOIL) {
            if (g_Input.action) {
                FireShotgun();
                ani++;
            }
        } else if (ani > AF_SG_RECOIL && ani < AF_SG_UNDRAW) {
            ani++;
            if (ani == AF_SG_UNDRAW) {
                ani = AF_SG_RECOIL;
            } else if (ani == AF_SG_RECOIL + 10) {
                Sound_Effect(SFX_LARA_RELOAD, &g_LaraItem->pos, SPM_NORMAL);
            }
        } else if (ani >= AF_SG_UNAIM && ani < AF_SG_END) {
            ani++;
            if (ani == AF_SG_END) {
                ani = AF_SG_AIM;
            }
        }
    } else {
        if (ani == AF_SG_AIM && g_Input.action) {
            ani++;
        } else if (ani > AF_SG_AIM && ani < AF_SG_DRAW) {
            ani++;
            if (ani == AF_SG_DRAW) {
                ani = g_Input.action ? AF_SG_RECOIL : AF_SG_UNAIM;
            }
        } else {
            if (ani == AF_SG_RECOIL) {
                if (g_Input.action) {
                    FireShotgun();
                    ani++;
                } else {
                    ani = AF_SG_UNAIM;
                }
            } else if (ani > AF_SG_RECOIL && ani < AF_SG_UNDRAW) {
                ani++;
                if (ani == AF_SG_RECOIL + 12 + 1) {
                    ani = AF_SG_AIM;
                } else if (ani == AF_SG_UNDRAW) {
                    ani = AF_SG_UNAIM;
                } else if (ani == AF_SG_RECOIL + 10) {
                    Sound_Effect(SFX_LARA_RELOAD, &g_LaraItem->pos, SPM_NORMAL);
                }
            } else if (ani >= AF_SG_UNAIM && ani < AF_SG_END) {
                ani++;
                if (ani == AF_SG_END) {
                    ani = AF_SG_AIM;
                }
            }
        }
    }

    g_Lara.right_arm.frame_number = ani;
    g_Lara.left_arm.frame_number = ani;
}

void FireShotgun()
{
    int32_t fired = 0;
    PHD_ANGLE angles[2];
    PHD_ANGLE dangles[2];

    angles[0] = g_Lara.left_arm.y_rot + g_LaraItem->pos.y_rot;
    angles[1] = g_Lara.left_arm.x_rot;

    for (int i = 0; i < SHOTGUN_AMMO_CLIP; i++) {
        dangles[0] = angles[0]
            + (int)((Random_GetControl() - 16384) * PELLET_SCATTER) / 65536;
        dangles[1] = angles[1]
            + (int)((Random_GetControl() - 16384) * PELLET_SCATTER) / 65536;
        if (FireWeapon(LGT_SHOTGUN, g_Lara.target, g_LaraItem, dangles)) {
            fired = 1;
        }
    }
    if (fired) {
        if (g_Config.enable_shotgun_flash) {
            g_Lara.right_arm.flash_gun = g_Weapons[LGT_SHOTGUN].flash_time;
        }
        Sound_Effect(
            g_Weapons[LGT_SHOTGUN].sample_num, &g_LaraItem->pos, SPM_NORMAL);
    }
}
