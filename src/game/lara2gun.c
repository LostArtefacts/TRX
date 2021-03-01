#include "game/effects.h"
#include "game/lara.h"
#include "game/vars.h"
#include "util.h"

void draw_pistols(int32_t weapon_type)
{
    int16_t ani = Lara.left_arm.frame_number;
    ani++;

    if (ani < AF_G_DRAW1 || ani > AF_G_DRAW2_L) {
        ani = AF_G_DRAW1;
    } else if (ani == AF_G_DRAW2) {
        draw_pistol_meshes(weapon_type);
        SoundEffect(6, &LaraItem->pos, 0);
    } else if (ani == AF_G_DRAW2_L) {
        ready_pistols();
        ani = AF_G_AIM;
    }

    Lara.left_arm.frame_number = ani;
    Lara.right_arm.frame_number = ani;
}

void undraw_pistols(int32_t weapon_type)
{
    int16_t anil = Lara.left_arm.frame_number;
    if (anil >= AF_G_RECOIL) {
        anil = AF_G_AIM_L;
    } else if (anil > AF_G_AIM && anil < AF_G_DRAW1) {
        Lara.left_arm.x_rot -= Lara.left_arm.x_rot / anil;
        Lara.left_arm.y_rot -= Lara.left_arm.y_rot / anil;
        anil--;
    } else if (anil == AF_G_AIM) {
        Lara.left_arm.x_rot = 0;
        Lara.left_arm.y_rot = 0;
        Lara.left_arm.z_rot = 0;
        anil = AF_G_DRAW2_L;
    } else if (anil > AF_G_DRAW1) {
        anil--;
        if (anil == AF_G_DRAW2) {
            undraw_pistol_mesh_left(weapon_type);
        }
    }
    Lara.left_arm.frame_number = anil;

    int16_t anir = Lara.right_arm.frame_number;
    if (anir >= AF_G_RECOIL) {
        anir = AF_G_AIM_L;
    } else if (anir > AF_G_AIM && anir < AF_G_DRAW1) {
        Lara.right_arm.x_rot -= Lara.right_arm.x_rot / anir;
        Lara.right_arm.y_rot -= Lara.right_arm.y_rot / anir;
        anir--;
    } else if (anir == AF_G_AIM) {
        Lara.right_arm.x_rot = 0;
        Lara.right_arm.y_rot = 0;
        Lara.right_arm.z_rot = 0;
        anir = AF_G_DRAW2_L;
    } else if (anir > AF_G_DRAW1) {
        anir--;
        if (anir == AF_G_DRAW2) {
            undraw_pistol_mesh_right(weapon_type);
        }
    }
    Lara.right_arm.frame_number = anir;

    if (anil == AF_G_DRAW1 && anir == AF_G_DRAW1) {
        Lara.left_arm.lock = 0;
        Lara.right_arm.lock = 0;
        Lara.left_arm.frame_number = AF_G_AIM;
        Lara.right_arm.frame_number = AF_G_AIM;
        Lara.gun_status = LGS_ARMLESS;
        Lara.target = NULL;
    }

    Lara.head_x_rot = (Lara.right_arm.x_rot + Lara.left_arm.x_rot) / 4;
    Lara.head_y_rot = (Lara.right_arm.y_rot + Lara.left_arm.y_rot) / 4;
    Lara.torso_x_rot = (Lara.right_arm.x_rot + Lara.left_arm.x_rot) / 4;
    Lara.torso_y_rot = (Lara.right_arm.y_rot + Lara.left_arm.y_rot) / 4;
}

void ready_pistols()
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

void draw_pistol_meshes(int32_t weapon_type)
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

void undraw_pistol_mesh_left(int32_t weapon_type)
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
    SoundEffect(7, &LaraItem->pos, 0);
}

void undraw_pistol_mesh_right(int32_t weapon_type)
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
    SoundEffect(7, &LaraItem->pos, 0);
}

void PistolHandler(int32_t weapon_type)
{
    WEAPON_INFO* winfo = &Weapons[weapon_type];

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
    WEAPON_INFO* winfo = &Weapons[weapon_type];

    int16_t anir = Lara.right_arm.frame_number;
    if (Lara.right_arm.lock || ((Input & IN_ACTION) && !Lara.target)) {
        if (anir >= AF_G_AIM && anir < AF_G_AIM_L) {
            anir++;
        } else if (anir == AF_G_AIM_L && (Input & IN_ACTION)) {
            angles[0] = Lara.right_arm.y_rot + LaraItem->pos.y_rot;
            angles[1] = Lara.right_arm.x_rot;
            if (FireWeapon(weapon_type, Lara.target, LaraItem, angles)) {
                Lara.right_arm.flash_gun = winfo->flash_time;
                SoundEffect(winfo->sample_num, &LaraItem->pos, 0);
            }
            anir = AF_G_RECOIL;
        } else if (anir >= AF_G_RECOIL) {
            anir++;
            if (anir == AF_G_RECOIL + winfo->recoil_frame) {
                anir = AF_G_AIM_L;
            }
        }
    } else if (anir >= AF_G_RECOIL) {
        anir = AF_G_AIM_L;
    } else if (anir > AF_G_AIM && anir <= AF_G_AIM_L) {
        anir--;
    }
    Lara.right_arm.frame_number = anir;

    int16_t anil = Lara.left_arm.frame_number;
    if (Lara.left_arm.lock || ((Input & IN_ACTION) && !Lara.target)) {
        if (anil >= AF_G_AIM && anil < AF_G_AIM_L) {
            ++anil;
        } else if (anil == AF_G_AIM_L && (Input & IN_ACTION)) {
            angles[0] = Lara.left_arm.y_rot + LaraItem->pos.y_rot;
            angles[1] = Lara.left_arm.x_rot;
            if (FireWeapon(weapon_type, Lara.target, LaraItem, angles)) {
                Lara.left_arm.flash_gun = winfo->flash_time;
                SoundEffect(winfo->sample_num, &LaraItem->pos, 0);
            }
            anil = AF_G_RECOIL;
        } else if (anil >= AF_G_RECOIL) {
            anil++;
            if (anil == AF_G_RECOIL + winfo->recoil_frame) {
                anil = AF_G_AIM_L;
            }
        }
    } else if (anil >= AF_G_RECOIL) {
        anil = AF_G_AIM_L;
    } else if (anil > AF_G_AIM && anil <= AF_G_AIM_L) {
        --anil;
    }
    Lara.left_arm.frame_number = anil;
}

void T1MInjectGameLaraGun2()
{
    INJECT(0x00426470, draw_pistols);
    INJECT(0x004265C0, undraw_pistols);
    INJECT(0x00426830, draw_pistol_meshes);
    INJECT(0x004268A0, PistolHandler);
    INJECT(0x004269D0, AnimatePistols);
}
