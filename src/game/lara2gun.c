#include "game/lara.h"

#include "game/sound.h"
#include "global/types.h"
#include "global/vars.h"
#include "util.h"

#include <stddef.h>
#include <stdint.h>

// original name: draw_pistols
void DrawPistols(int32_t weapon_type)
{
    int16_t ani = Lara.left_arm.frame_number;
    ani++;

    if (ani < (AF_G_DRAW1  * ANIM_SCALE) || ani > (AF_G_DRAW2_L  * ANIM_SCALE)) {
        ani = (AF_G_DRAW1  * ANIM_SCALE);
    } else if (ani == (AF_G_DRAW2  * ANIM_SCALE)) {
        DrawPistolMeshes(weapon_type);
        SoundEffect(SFX_LARA_DRAW, &LaraItem->pos, SPM_NORMAL);
    } else if (ani == (AF_G_DRAW2_L  * ANIM_SCALE)) {
        ReadyPistols();
        ani = (AF_G_AIM  * ANIM_SCALE);
    }

    Lara.left_arm.frame_number = ani;
    Lara.right_arm.frame_number = ani;
}

// original name: undraw_pistols
void UndrawPistols(int32_t weapon_type)
{
    int16_t anil = Lara.left_arm.frame_number;
    if (anil >= (AF_G_RECOIL * ANIM_SCALE)) {
        anil = (AF_G_AIM_L  * ANIM_SCALE);
    } else if (anil > (AF_G_AIM  * ANIM_SCALE) && anil < (AF_G_DRAW1  * ANIM_SCALE)) {
        Lara.left_arm.x_rot -= Lara.left_arm.x_rot / anil;
        Lara.left_arm.y_rot -= Lara.left_arm.y_rot / anil;
        anil--;
    } else if (anil == (AF_G_AIM  * ANIM_SCALE)) {
        Lara.left_arm.x_rot = 0;
        Lara.left_arm.y_rot = 0;
        Lara.left_arm.z_rot = 0;
        anil = (AF_G_DRAW2_L  * ANIM_SCALE);
    } else if (anil > (AF_G_DRAW1  * ANIM_SCALE)) {
        anil--;
        if (anil == (AF_G_DRAW2  * ANIM_SCALE)) {
            UndrawPistolMeshLeft(weapon_type);
        }
    }
    Lara.left_arm.frame_number = anil;

    int16_t anir = Lara.right_arm.frame_number;
    if (anir >= (AF_G_RECOIL * ANIM_SCALE)) {
        anir = (AF_G_AIM_L  * ANIM_SCALE);
    } else if (anir > (AF_G_AIM  * ANIM_SCALE) && anir < (AF_G_DRAW1  * ANIM_SCALE)) {
        Lara.right_arm.x_rot -= Lara.right_arm.x_rot / anir;
        Lara.right_arm.y_rot -= Lara.right_arm.y_rot / anir;
        anir--;
    } else if (anir == (AF_G_AIM  * ANIM_SCALE)) {
        Lara.right_arm.x_rot = 0;
        Lara.right_arm.y_rot = 0;
        Lara.right_arm.z_rot = 0;
        anir = (AF_G_DRAW2_L  * ANIM_SCALE);
    } else if (anir > (AF_G_DRAW1  * ANIM_SCALE)) {
        anir--;
        if (anir == (AF_G_DRAW2  * ANIM_SCALE)) {
            UndrawPistolMeshRight(weapon_type);
        }
    }
    Lara.right_arm.frame_number = anir;

    if (anil == (AF_G_DRAW1  * ANIM_SCALE) && anir == (AF_G_DRAW1  * ANIM_SCALE)) {
        Lara.left_arm.lock = 0;
        Lara.right_arm.lock = 0;
        Lara.left_arm.frame_number = (AF_G_AIM  * ANIM_SCALE);
        Lara.right_arm.frame_number = (AF_G_AIM  * ANIM_SCALE);
        Lara.gun_status = LGS_ARMLESS;
        Lara.target = NULL;
    }

    Lara.head_x_rot = (Lara.right_arm.x_rot + Lara.left_arm.x_rot) / 4;
    Lara.head_y_rot = (Lara.right_arm.y_rot + Lara.left_arm.y_rot) / 4;
    Lara.torso_x_rot = (Lara.right_arm.x_rot + Lara.left_arm.x_rot) / 4;
    Lara.torso_y_rot = (Lara.right_arm.y_rot + Lara.left_arm.y_rot) / 4;
}

// original name: ready_pistols
void ReadyPistols()
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
    Lara.right_arm.frame_base = Objects[O_PISTOLS].frame_base;
    Lara.left_arm.frame_base = Objects[O_PISTOLS].frame_base;
}

// original name: draw_pistol_meshes
void DrawPistolMeshes(int32_t weapon_type)
{
    int16_t object_num = O_PISTOLS;
    if (weapon_type == LGT_MAGNUMS) {
        object_num = O_MAGNUM;
    } else if (weapon_type == LGT_UZIS) {
        object_num = O_UZI;
    }

    Lara.mesh_ptrs[LM_HAND_L] =
        Meshes[Objects[object_num].mesh_index + LM_HAND_L];
    Lara.mesh_ptrs[LM_HAND_R] =
        Meshes[Objects[object_num].mesh_index + LM_HAND_R];
    Lara.mesh_ptrs[LM_THIGH_L] =
        Meshes[Objects[O_LARA].mesh_index + LM_THIGH_L];
    Lara.mesh_ptrs[LM_THIGH_R] =
        Meshes[Objects[O_LARA].mesh_index + LM_THIGH_R];
}

// original name: undraw_pistol_mesh_left
void UndrawPistolMeshLeft(int32_t weapon_type)
{
    int16_t object_num = O_PISTOLS;
    if (weapon_type == LGT_MAGNUMS) {
        object_num = O_MAGNUM;
    } else if (weapon_type == LGT_UZIS) {
        object_num = O_UZI;
    }
    Lara.mesh_ptrs[LM_THIGH_L] =
        Meshes[Objects[object_num].mesh_index + LM_THIGH_L];
    Lara.mesh_ptrs[LM_HAND_L] = Meshes[Objects[O_LARA].mesh_index + LM_HAND_L];
    SoundEffect(SFX_LARA_HOLSTER, &LaraItem->pos, SPM_NORMAL);
}

// original name: undraw_pistol_mesh_right
void UndrawPistolMeshRight(int32_t weapon_type)
{
    int16_t object_num = O_PISTOLS;
    if (weapon_type == LGT_MAGNUMS) {
        object_num = O_MAGNUM;
    } else if (weapon_type == LGT_UZIS) {
        object_num = O_UZI;
    }
    Lara.mesh_ptrs[LM_THIGH_R] =
        Meshes[Objects[object_num].mesh_index + LM_THIGH_R];
    Lara.mesh_ptrs[LM_HAND_R] = Meshes[Objects[O_LARA].mesh_index + LM_HAND_R];
    SoundEffect(SFX_LARA_HOLSTER, &LaraItem->pos, SPM_NORMAL);
}

void PistolHandler(int32_t weapon_type)
{
    WEAPON_INFO *winfo = &Weapons[weapon_type];

    if (Input & IN_ACTION) {
        LaraTargetInfo(winfo);
    } else {
        Lara.target = NULL;
    }
    if (!Lara.target) {
        LaraGetNewTarget(winfo);
    }

    AimWeapon(winfo, &Lara.left_arm);
    AimWeapon(winfo, &Lara.right_arm);

    if (Lara.left_arm.lock && !Lara.right_arm.lock) {
        Lara.head_x_rot = Lara.left_arm.x_rot / 2;
        Lara.head_y_rot = Lara.left_arm.y_rot / 2;
        Lara.torso_x_rot = Lara.left_arm.x_rot / 2;
        Lara.torso_y_rot = Lara.left_arm.y_rot / 2;
    } else if (!Lara.left_arm.lock && Lara.right_arm.lock) {
        Lara.head_x_rot = Lara.right_arm.x_rot / 2;
        Lara.head_y_rot = Lara.right_arm.y_rot / 2;
        Lara.torso_x_rot = Lara.right_arm.x_rot / 2;
        Lara.torso_y_rot = Lara.right_arm.y_rot / 2;
    } else if (Lara.left_arm.lock && Lara.right_arm.lock) {
        Lara.head_x_rot = (Lara.right_arm.x_rot + Lara.left_arm.x_rot) / 4;
        Lara.head_y_rot = (Lara.right_arm.y_rot + Lara.left_arm.y_rot) / 4;
        Lara.torso_x_rot = (Lara.right_arm.x_rot + Lara.left_arm.x_rot) / 4;
        Lara.torso_y_rot = (Lara.right_arm.y_rot + Lara.left_arm.y_rot) / 4;
    }

    AnimatePistols(weapon_type);
}

void AnimatePistols(int32_t weapon_type)
{
    PHD_ANGLE angles[2];
    WEAPON_INFO *winfo = &Weapons[weapon_type];

    int16_t anir = Lara.right_arm.frame_number;
    if (Lara.right_arm.lock || ((Input & IN_ACTION) && !Lara.target)) {
        if (anir >= (AF_G_AIM  * ANIM_SCALE) && anir < (AF_G_AIM_L  * ANIM_SCALE)) {
            anir++;
        } else if (anir == (AF_G_AIM_L  * ANIM_SCALE) && (Input & IN_ACTION)) {
            angles[0] = Lara.right_arm.y_rot + LaraItem->pos.y_rot;
            angles[1] = Lara.right_arm.x_rot;
            if (FireWeapon(weapon_type, Lara.target, LaraItem, angles)) {
                Lara.right_arm.flash_gun = winfo->flash_time * ANIM_SCALE;
                SoundEffect(winfo->sample_num, &LaraItem->pos, SPM_NORMAL);
            }
            anir = (AF_G_RECOIL * ANIM_SCALE);
        } else if (anir >= (AF_G_RECOIL * ANIM_SCALE)) {
            anir++;
            if (anir == (AF_G_RECOIL * ANIM_SCALE) + (winfo->recoil_frame * ANIM_SCALE)) {
                anir = (AF_G_AIM_L  * ANIM_SCALE);
            }
        }
    } else if (anir >= (AF_G_RECOIL * ANIM_SCALE)) {
        anir = (AF_G_AIM_L  * ANIM_SCALE);
    } else if (anir > (AF_G_AIM  * ANIM_SCALE) && anir <= (AF_G_AIM_L  * ANIM_SCALE)) {
        anir--;
    }
    Lara.right_arm.frame_number = anir;

    int16_t anil = Lara.left_arm.frame_number;
    if (Lara.left_arm.lock || ((Input & IN_ACTION) && !Lara.target)) {
        if (anil >= (AF_G_AIM  * ANIM_SCALE) && anil < (AF_G_AIM_L  * ANIM_SCALE)) {
            anil++;
        } else if (anil == (AF_G_AIM_L  * ANIM_SCALE) && (Input & IN_ACTION)) {
            angles[0] = Lara.left_arm.y_rot + LaraItem->pos.y_rot;
            angles[1] = Lara.left_arm.x_rot;
            if (FireWeapon(weapon_type, Lara.target, LaraItem, angles)) {
                Lara.left_arm.flash_gun = winfo->flash_time * ANIM_SCALE;
                SoundEffect(winfo->sample_num, &LaraItem->pos, SPM_NORMAL);
            }
            anil = (AF_G_RECOIL * ANIM_SCALE);
        } else if (anil >= (AF_G_RECOIL * ANIM_SCALE)) {
            anil++;
            if (anil == (AF_G_RECOIL * ANIM_SCALE) + (winfo->recoil_frame * ANIM_SCALE)) {
                anil = (AF_G_AIM_L  * ANIM_SCALE);
            }
        }
    } else if (anil >= (AF_G_RECOIL * ANIM_SCALE)) {
        anil = (AF_G_AIM_L  * ANIM_SCALE);
    } else if (anil > (AF_G_AIM  * ANIM_SCALE) && anil <= (AF_G_AIM_L  * ANIM_SCALE)) {
        anil--;
    }
    Lara.left_arm.frame_number = anil;
}

void T1MInjectGameLaraGun2()
{
    INJECT(0x00426470, DrawPistols);
    INJECT(0x004265C0, UndrawPistols);
    INJECT(0x00426830, DrawPistolMeshes);
    INJECT(0x004268A0, PistolHandler);
    INJECT(0x004269D0, AnimatePistols);
}
