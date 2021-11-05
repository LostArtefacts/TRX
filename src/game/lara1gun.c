#include "game/lara.h"

#include "config.h"
#include "game/game.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <stddef.h>
#include <stdint.h>

void DrawShotgun()
{
    int16_t ani = Lara.left_arm.frame_number;
    ani++;

    if (ani < AF_SG_DRAW || ani > AF_SG_RECOIL) {
        ani = AF_SG_DRAW;
    } else if (ani == AF_SG_DRAW + 10) {
        DrawShotgunMeshes();
        Sound_Effect(SFX_LARA_DRAW, &LaraItem->pos, SPM_NORMAL);
    } else if (ani == AF_SG_RECOIL) {
        ReadyShotgun();
        ani = AF_SG_AIM;
    }
    Lara.left_arm.frame_number = ani;
    Lara.right_arm.frame_number = ani;
}

void UndrawShotgun()
{
    int16_t ani = ani = Lara.left_arm.frame_number;

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
            Sound_Effect(SFX_LARA_DRAW, &LaraItem->pos, SPM_NORMAL);
        } else if (ani == AF_SG_UNAIM) {
            ani = AF_SG_AIM;
            Lara.gun_status = LGS_ARMLESS;
            Lara.target = NULL;
            Lara.right_arm.lock = 0;
            Lara.left_arm.lock = 0;
        }
    }

    Lara.head_x_rot = 0;
    Lara.head_y_rot = 0;
    Lara.torso_x_rot += Lara.torso_x_rot / -2;
    Lara.torso_y_rot += Lara.torso_y_rot / -2;
    Lara.right_arm.frame_number = ani;
    Lara.left_arm.frame_number = ani;
}

void DrawShotgunMeshes()
{
    Lara.mesh_ptrs[LM_HAND_L] =
        Meshes[Objects[O_SHOTGUN].mesh_index + LM_HAND_L];
    Lara.mesh_ptrs[LM_HAND_R] =
        Meshes[Objects[O_SHOTGUN].mesh_index + LM_HAND_R];
    Lara.mesh_ptrs[LM_TORSO] = Meshes[Objects[O_LARA].mesh_index + LM_TORSO];
}

void UndrawShotgunMeshes()
{
    Lara.mesh_ptrs[LM_HAND_L] = Meshes[Objects[O_LARA].mesh_index + LM_HAND_L];
    Lara.mesh_ptrs[LM_HAND_R] = Meshes[Objects[O_LARA].mesh_index + LM_HAND_R];
    Lara.mesh_ptrs[LM_TORSO] = Meshes[Objects[O_SHOTGUN].mesh_index + LM_TORSO];
}

void ReadyShotgun()
{
    Lara.gun_status = LGS_READY;
    Lara.left_arm.x_rot = 0;
    Lara.left_arm.y_rot = 0;
    Lara.left_arm.z_rot = 0;
    Lara.left_arm.lock = 0;
    Lara.right_arm.x_rot = 0;
    Lara.right_arm.y_rot = 0;
    Lara.right_arm.z_rot = 0;
    Lara.right_arm.lock = 0;
    Lara.head_x_rot = 0;
    Lara.head_y_rot = 0;
    Lara.torso_x_rot = 0;
    Lara.torso_y_rot = 0;
    Lara.target = NULL;
    Lara.right_arm.frame_base = Objects[O_SHOTGUN].frame_base;
    Lara.left_arm.frame_base = Objects[O_SHOTGUN].frame_base;
}

void RifleHandler(int32_t weapon_type)
{
    WEAPON_INFO *winfo = &Weapons[LGT_SHOTGUN];

    if (Input.action) {
        LaraTargetInfo(winfo);
    } else {
        Lara.target = NULL;
    }
    if (!Lara.target) {
        LaraGetNewTarget(winfo);
    }

    AimWeapon(winfo, &Lara.left_arm);

    if (Lara.left_arm.lock) {
        Lara.torso_y_rot = Lara.left_arm.y_rot / 2;
        Lara.torso_x_rot = Lara.left_arm.x_rot / 2;
        Lara.head_x_rot = 0;
        Lara.head_y_rot = 0;
    }

    AnimateShotgun();
}

void AnimateShotgun()
{
    int16_t ani = Lara.left_arm.frame_number;
    if (Lara.left_arm.lock) {
        if (ani >= AF_SG_AIM && ani < AF_SG_DRAW) {
            ani++;
            if (ani == AF_SG_DRAW) {
                ani = AF_SG_RECOIL;
            }
        } else if (ani == AF_SG_RECOIL) {
            if (Input.action) {
                FireShotgun();
                ani++;
            }
        } else if (ani > AF_SG_RECOIL && ani < AF_SG_UNDRAW) {
            ani++;
            if (ani == AF_SG_UNDRAW) {
                ani = AF_SG_RECOIL;
            } else if (ani == AF_SG_RECOIL + 10) {
                Sound_Effect(SFX_LARA_RELOAD, &LaraItem->pos, SPM_NORMAL);
            }
        } else if (ani >= AF_SG_UNAIM && ani < AF_SG_END) {
            ani++;
            if (ani == AF_SG_END) {
                ani = AF_SG_AIM;
            }
        }
    } else {
        if (ani == AF_SG_AIM && Input.action) {
            ani++;
        } else if (ani > AF_SG_AIM && ani < AF_SG_DRAW) {
            ani++;
            if (ani == AF_SG_DRAW) {
                ani = Input.action ? AF_SG_RECOIL : AF_SG_UNAIM;
            }
        } else {
            if (ani == AF_SG_RECOIL) {
                if (Input.action) {
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
                    Sound_Effect(SFX_LARA_RELOAD, &LaraItem->pos, SPM_NORMAL);
                }
            } else if (ani >= AF_SG_UNAIM && ani < AF_SG_END) {
                ani++;
                if (ani == AF_SG_END) {
                    ani = AF_SG_AIM;
                }
            }
        }
    }

    Lara.right_arm.frame_number = ani;
    Lara.left_arm.frame_number = ani;
}

void FireShotgun()
{
    int32_t fired = 0;
    PHD_ANGLE angles[2];
    PHD_ANGLE dangles[2];

    angles[0] = Lara.left_arm.y_rot + LaraItem->pos.y_rot;
    angles[1] = Lara.left_arm.x_rot;

    for (int i = 0; i < SHOTGUN_AMMO_CLIP; i++) {
        dangles[0] = angles[0]
            + (int)((GetRandomControl() - 16384) * PELLET_SCATTER) / 65536;
        dangles[1] = angles[1]
            + (int)((GetRandomControl() - 16384) * PELLET_SCATTER) / 65536;
        if (FireWeapon(LGT_SHOTGUN, Lara.target, LaraItem, dangles)) {
            fired = 1;
        }
    }
    if (fired) {
        if (T1MConfig.enable_shotgun_flash) {
            Lara.right_arm.flash_gun = Weapons[LGT_SHOTGUN].flash_time;
        }
        Sound_Effect(
            Weapons[LGT_SHOTGUN].sample_num, &LaraItem->pos, SPM_NORMAL);
    }
}
