#include "game/data.h"
#include "game/effects.h"
#include "game/lara.h"
#include "util.h"

void __cdecl draw_pistols(int32_t weapon_type)
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

void __cdecl undraw_pistols(int32_t weapon_type)
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

void __cdecl ready_pistols()
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

void __cdecl draw_pistol_meshes(int32_t weapon_type)
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

void __cdecl undraw_pistol_mesh_left(int32_t weapon_type)
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

void __cdecl undraw_pistol_mesh_right(int32_t weapon_type)
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

void Tomb1MInjectGameLaraGun2()
{
    INJECT(0x00426470, draw_pistols);
    INJECT(0x004265C0, undraw_pistols);
}
