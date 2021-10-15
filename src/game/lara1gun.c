#include "game/lara.h"

#include "config.h"
#include "game/game.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "util.h"

#include <stddef.h>
#include <stdint.h>

// original name: draw_shotgun
void DrawShotgun()
{
    int16_t ani = Lara.left_arm.frame_number;
    ani++;

    if (ani < AF_SG_DRAW * AnimScale || ani > AF_SG_RECOIL * AnimScale) {
        ani = AF_SG_DRAW * AnimScale;
    } else if (ani == AF_SG_DRAW * AnimScale + 10 * AnimScale) {
        DrawShotgunMeshes();
        SoundEffect(SFX_LARA_DRAW, &LaraItem->pos, SPM_NORMAL);
    } else if (ani == AF_SG_RECOIL * AnimScale) {
        ReadyShotgun();
        ani = AF_SG_AIM * AnimScale;
    }
    Lara.left_arm.frame_number = ani;
    Lara.right_arm.frame_number = ani;
}

// origianl name: undraw_shotgun
void UndrawShotgun()
{
    int16_t ani = ani = Lara.left_arm.frame_number;

    if (ani == AF_SG_AIM * AnimScale) {
        ani = AF_SG_UNDRAW * AnimScale;
    } else if (
        ani >= AF_SG_AIM * AnimScale && ani < AF_SG_DRAW * AnimScale) {
        ani++;
        if (ani == AF_SG_DRAW * AnimScale) {
            ani = AF_SG_UNAIM * AnimScale;
        }
    } else if (ani == AF_SG_RECOIL * AnimScale) {
        ani = AF_SG_UNAIM * AnimScale;
    } else if (
        ani >= AF_SG_RECOIL * AnimScale
        && ani < AF_SG_UNDRAW * AnimScale) {
        ani++;
        if (ani == AF_SG_RECOIL * AnimScale + 12 * AnimScale) {
            ani = AF_SG_AIM * AnimScale;
        } else if (ani == AF_SG_UNDRAW * AnimScale) {
            ani = AF_SG_UNAIM * AnimScale;
        }
    } else if (
        ani >= AF_SG_UNAIM * AnimScale && ani < AF_SG_END * AnimScale) {
        ani++;
        if (ani == AF_SG_END * AnimScale) {
            ani = AF_SG_UNDRAW * AnimScale;
        }
    } else if (
        ani >= AF_SG_UNDRAW * AnimScale
        && ani < AF_SG_UNAIM * AnimScale) {
        ani++;
        if (ani == AF_SG_UNDRAW * AnimScale + 20 * AnimScale) {
            UndrawShotgunMeshes();
            SoundEffect(SFX_LARA_DRAW, &LaraItem->pos, SPM_NORMAL);
        } else if (ani == AF_SG_UNAIM * AnimScale) {
            ani = AF_SG_AIM * AnimScale;
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

// original name: draw_shotgun_meshes
void DrawShotgunMeshes()
{
    Lara.mesh_ptrs[LM_HAND_L] =
        Meshes[Objects[O_SHOTGUN].mesh_index + LM_HAND_L];
    Lara.mesh_ptrs[LM_HAND_R] =
        Meshes[Objects[O_SHOTGUN].mesh_index + LM_HAND_R];
    Lara.mesh_ptrs[LM_TORSO] = Meshes[Objects[O_LARA].mesh_index + LM_TORSO];
}

// original name: undraw_shotgun_meshes
void UndrawShotgunMeshes()
{
    Lara.mesh_ptrs[LM_HAND_L] = Meshes[Objects[O_LARA].mesh_index + LM_HAND_L];
    Lara.mesh_ptrs[LM_HAND_R] = Meshes[Objects[O_LARA].mesh_index + LM_HAND_R];
    Lara.mesh_ptrs[LM_TORSO] = Meshes[Objects[O_SHOTGUN].mesh_index + LM_TORSO];
}

// original name: ReadyShotgun
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

    if (Input & IN_ACTION) {
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
        if (ani >= AF_SG_AIM * AnimScale
            && ani < AF_SG_DRAW * AnimScale) {
            ani++;
            if (ani == AF_SG_DRAW * AnimScale) {
                ani = AF_SG_RECOIL * AnimScale;
            }
        } else if (ani == AF_SG_RECOIL * AnimScale) {
            if (Input & IN_ACTION) {
                FireShotgun();
                ani++;
            }
        } else if (
            ani > AF_SG_RECOIL * AnimScale
            && ani < AF_SG_UNDRAW * AnimScale) {
            ani++;
            if (ani == AF_SG_UNDRAW * AnimScale) {
                ani = AF_SG_RECOIL * AnimScale;
            } else if (ani == AF_SG_RECOIL * AnimScale + 10 * AnimScale) {
                SoundEffect(SFX_LARA_RELOAD, &LaraItem->pos, SPM_NORMAL);
            }
        } else if (
            ani >= AF_SG_UNAIM * AnimScale
            && ani < AF_SG_END * AnimScale) {
            ani++;
            if (ani == AF_SG_END * AnimScale) {
                ani = AF_SG_AIM * AnimScale;
            }
        }
    } else {
        if (ani == AF_SG_AIM * AnimScale && (Input & IN_ACTION)) {
            ani++;
        } else if (
            ani > AF_SG_AIM * AnimScale && ani < AF_SG_DRAW * AnimScale) {
            ani++;
            if (ani == AF_SG_DRAW * AnimScale) {
                if (Input & IN_ACTION) {
                    ani = AF_SG_RECOIL * AnimScale;
                } else {
                    ani = AF_SG_UNAIM * AnimScale;
                }
            }
        } else {
            if (ani == AF_SG_RECOIL * AnimScale) {
                if (Input & IN_ACTION) {
                    FireShotgun();
                    ani++;
                } else {
                    ani = AF_SG_UNAIM * AnimScale;
                }
            } else if (
                ani > AF_SG_RECOIL * AnimScale
                && ani < AF_SG_UNDRAW * AnimScale) {
                ani++;
                if (ani == ((AF_SG_RECOIL + 12 + 1) * AnimScale)) {
                    ani = AF_SG_AIM * AnimScale;
                } else if (ani == AF_SG_UNDRAW * AnimScale) {
                    ani = AF_SG_UNAIM * AnimScale;
                } else if (ani == (AF_SG_RECOIL + 10) * AnimScale) {
                    SoundEffect(SFX_LARA_RELOAD, &LaraItem->pos, SPM_NORMAL);
                }
            } else if (
                ani >= AF_SG_UNAIM * AnimScale
                && ani < AF_SG_END * AnimScale) {
                ani++;
                if (ani == AF_SG_END * AnimScale) {
                    ani = AF_SG_AIM * AnimScale;
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
            Lara.right_arm.flash_gun =
                Weapons[LGT_SHOTGUN].flash_time * AnimScale;
        }
        SoundEffect(
            Weapons[LGT_SHOTGUN].sample_num, &LaraItem->pos, SPM_NORMAL);
    }
}

void T1MInjectGameLaraGun1()
{
    INJECT(0x00425E30, DrawShotgun);
    INJECT(0x00425F50, UndrawShotgun);
    INJECT(0x004260F0, RifleHandler);
}
