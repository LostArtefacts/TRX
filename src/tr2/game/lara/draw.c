#include "game/lara/draw.h"

#include "game/items.h"
#include "game/matrix.h"
#include "game/output.h"
#include "game/random.h"
#include "global/funcs.h"
#include "global/vars.h"

void __cdecl Lara_Draw(const ITEM *const item)
{
    FRAME_INFO *frame;
    MATRIX saved_matrix;

    const int32_t top = g_PhdWinTop;
    const int32_t left = g_PhdWinLeft;
    const int32_t right = g_PhdWinRight;
    const int32_t bottom = g_PhdWinBottom;

    g_PhdWinTop = 0;
    g_PhdWinLeft = 0;
    g_PhdWinBottom = g_PhdWinMaxY;
    g_PhdWinRight = g_PhdWinMaxX;

    FRAME_INFO *frames[2];
    if (g_Lara.hit_direction < 0) {
        int32_t rate;
        const int32_t frac = Item_GetFrames(item, frames, &rate);
        if (frac) {
            Lara_Draw_I(item, frames[0], frames[1], frac, rate);
            goto finish;
        }
    }

    const OBJECT *const object = &g_Objects[item->object_id];
    if (g_Lara.hit_direction < 0) {
        frame = frames[0];
    } else {
        // clang-format off
        LARA_ANIMATION anim;
        switch (g_Lara.hit_direction) {
        case DIR_EAST:  anim = LA_HIT_LEFT; break;
        case DIR_SOUTH: anim = LA_HIT_BACK; break;
        case DIR_WEST:  anim = LA_HIT_RIGHT; break;
        default:        anim = LA_HIT_FRONT; break;
        }
        // clang-format on
        frame = (FRAME_INFO *)(g_Anims[anim].frame_ptr
                               + g_Lara.hit_frame
                                   * (g_Anims[anim].interpolation >> 8));
    }

    if (g_Lara.skidoo == NO_ITEM) {
        S_PrintShadow(object->shadow_size, &frame->bounds, item);
    }

    saved_matrix = *g_MatrixPtr;

    Matrix_Push();
    Matrix_TranslateAbs(item->pos.x, item->pos.y, item->pos.z);
    Matrix_RotYXZ(item->rot.y, item->rot.x, item->rot.z);
    const int32_t clip = S_GetObjectBounds(&frame->bounds);
    if (!clip) {
        Matrix_Pop();
        return;
    }

    Matrix_Push();
    Output_CalculateObjectLighting(item, frame);

    const int32_t *bone = &g_AnimBones[object->bone_idx];
    const int16_t *mesh_rots = frame->mesh_rots;
    const int16_t *mesh_rots_c;

    Matrix_TranslateRel(frame->offset.x, frame->offset.y, frame->offset.z);
    Matrix_RotYXZsuperpack(&mesh_rots, 0);
    Output_InsertPolygons(g_Lara.mesh_ptrs[LM_HIPS], clip);

    Matrix_Push();
    Matrix_TranslateRel(bone[1], bone[2], bone[3]);
    Matrix_RotYXZsuperpack(&mesh_rots, 0);
    Output_InsertPolygons(g_Lara.mesh_ptrs[LM_THIGH_L], clip);

    Matrix_TranslateRel(bone[5], bone[6], bone[7]);
    Matrix_RotYXZsuperpack(&mesh_rots, 0);
    Output_InsertPolygons(g_Lara.mesh_ptrs[LM_CALF_L], clip);

    Matrix_TranslateRel(bone[9], bone[10], bone[11]);
    Matrix_RotYXZsuperpack(&mesh_rots, 0);
    Output_InsertPolygons(g_Lara.mesh_ptrs[LM_FOOT_L], clip);
    Matrix_Pop();

    Matrix_Push();
    Matrix_TranslateRel(bone[13], bone[14], bone[15]);
    Matrix_RotYXZsuperpack(&mesh_rots, 0);
    Output_InsertPolygons(g_Lara.mesh_ptrs[LM_THIGH_R], clip);

    Matrix_TranslateRel(bone[17], bone[18], bone[19]);
    Matrix_RotYXZsuperpack(&mesh_rots, 0);
    Output_InsertPolygons(g_Lara.mesh_ptrs[LM_CALF_R], clip);

    Matrix_TranslateRel(bone[21], bone[22], bone[23]);
    Matrix_RotYXZsuperpack(&mesh_rots, 0);
    Output_InsertPolygons(g_Lara.mesh_ptrs[LM_FOOT_R], clip);
    Matrix_Pop();

    Matrix_TranslateRel(bone[25], bone[26], bone[27]);
    if (g_Lara.weapon_item != NO_ITEM && g_Lara.gun_type == LGT_M16
        && (g_Items[g_Lara.weapon_item].current_anim_state == 0
            || g_Items[g_Lara.weapon_item].current_anim_state == 2
            || g_Items[g_Lara.weapon_item].current_anim_state == 4)) {
        mesh_rots =
            &g_Lara.right_arm.frame_base
                 [g_Lara.right_arm.frame_num
                      * (g_Anims[g_Lara.right_arm.anim_num].interpolation >> 8)
                  + FBBOX_ROT];
        Matrix_RotYXZsuperpack(&mesh_rots, 7);
    } else {
        Matrix_RotYXZsuperpack(&mesh_rots, 0);
    }

    Matrix_RotYXZ(g_Lara.torso_y_rot, g_Lara.torso_x_rot, g_Lara.torso_z_rot);
    Output_InsertPolygons(g_Lara.mesh_ptrs[LM_TORSO], clip);

    Matrix_Push();
    Matrix_TranslateRel(bone[53], bone[54], bone[55]);
    mesh_rots_c = mesh_rots;
    Matrix_RotYXZsuperpack(&mesh_rots, 6);
    mesh_rots = mesh_rots_c;
    Matrix_RotYXZ(g_Lara.head_y_rot, g_Lara.head_x_rot, g_Lara.head_z_rot);
    Output_InsertPolygons(g_Lara.mesh_ptrs[LM_HEAD], clip);

    *g_MatrixPtr = saved_matrix;
    DrawHair();

    Matrix_Pop();

    if (g_Lara.back_gun) {
        Matrix_Push();
        const int32_t *bone_c =
            &g_AnimBones[g_Objects[g_Lara.back_gun].bone_idx];
        Matrix_TranslateRel(bone_c[53], bone_c[54], bone_c[55]);
        mesh_rots_c = g_Objects[g_Lara.back_gun].frame_base + FBBOX_ROT;
        Matrix_RotYXZsuperpack(&mesh_rots_c, 14);
        Output_InsertPolygons(
            g_Meshes[g_Objects[g_Lara.back_gun].mesh_idx + LM_HEAD], clip);
        Matrix_Pop();
    }

    LARA_GUN_TYPE gun_type = LGT_UNARMED;
    if (g_Lara.gun_status == LGS_READY || g_Lara.gun_status == LGS_SPECIAL
        || g_Lara.gun_status == LGS_DRAW || g_Lara.gun_status == LGS_UNDRAW) {
        gun_type = g_Lara.gun_type;
    }

    switch (gun_type) {
    case LGT_UNARMED:
    case LGT_FLARE:
        Matrix_Push();
        Matrix_TranslateRel(bone[29], bone[30], bone[31]);
        Matrix_RotYXZsuperpack(&mesh_rots, 0);
        Output_InsertPolygons(g_Lara.mesh_ptrs[LM_UARM_R], clip);

        Matrix_TranslateRel(bone[33], bone[34], bone[35]);
        Matrix_RotYXZsuperpack(&mesh_rots, 0);
        Output_InsertPolygons(g_Lara.mesh_ptrs[LM_LARM_R], clip);

        Matrix_TranslateRel(bone[37], bone[38], bone[39]);
        Matrix_RotYXZsuperpack(&mesh_rots, 0);
        Output_InsertPolygons(g_Lara.mesh_ptrs[LM_HAND_R], clip);
        Matrix_Pop();

        Matrix_Push();
        Matrix_TranslateRel(bone[41], bone[42], bone[43]);
        if (g_Lara.flare_control_left) {
            mesh_rots =
                &g_Lara.left_arm.frame_base
                     [(g_Anims[g_Lara.left_arm.anim_num].interpolation >> 8)
                          * (g_Lara.left_arm.frame_num
                             - g_Anims[g_Lara.left_arm.anim_num].frame_base)
                      + FBBOX_ROT];
            Matrix_RotYXZsuperpack(&mesh_rots, 11);
        } else {
            Matrix_RotYXZsuperpack(&mesh_rots, 0);
        }
        Output_InsertPolygons(g_Lara.mesh_ptrs[LM_UARM_L], clip);
        Matrix_TranslateRel(bone[45], bone[46], bone[47]);
        Matrix_RotYXZsuperpack(&mesh_rots, 0);
        Output_InsertPolygons(g_Lara.mesh_ptrs[LM_LARM_L], clip);
        Matrix_TranslateRel(bone[49], bone[50], bone[51]);
        Matrix_RotYXZsuperpack(&mesh_rots, 0);
        Output_InsertPolygons(g_Lara.mesh_ptrs[LM_HAND_L], clip);
        if (g_Lara.gun_type == LGT_FLARE && g_Lara.left_arm.flash_gun) {
            Gun_DrawFlash(LGT_FLARE, clip);
        }

        Matrix_Pop();
        break;

    case LGT_PISTOLS:
    case LGT_MAGNUMS:
    case LGT_UZIS:
        Matrix_Push();
        Matrix_TranslateRel(bone[29], bone[30], bone[31]);

        g_MatrixPtr->_00 = g_MatrixPtr[-2]._00;
        g_MatrixPtr->_01 = g_MatrixPtr[-2]._01;
        g_MatrixPtr->_02 = g_MatrixPtr[-2]._02;
        g_MatrixPtr->_10 = g_MatrixPtr[-2]._10;
        g_MatrixPtr->_11 = g_MatrixPtr[-2]._11;
        g_MatrixPtr->_12 = g_MatrixPtr[-2]._12;
        g_MatrixPtr->_20 = g_MatrixPtr[-2]._20;
        g_MatrixPtr->_21 = g_MatrixPtr[-2]._21;
        g_MatrixPtr->_22 = g_MatrixPtr[-2]._22;

        Matrix_RotYXZ(
            g_Lara.right_arm.rot.y, g_Lara.right_arm.rot.x,
            g_Lara.right_arm.rot.z);
        mesh_rots =
            &g_Lara.right_arm.frame_base
                 [(g_Anims[g_Lara.right_arm.anim_num].interpolation >> 8)
                      * (g_Lara.right_arm.frame_num
                         - g_Anims[g_Lara.right_arm.anim_num].frame_base)
                  + FBBOX_ROT];
        Matrix_RotYXZsuperpack(&mesh_rots, 8);
        Output_InsertPolygons(g_Lara.mesh_ptrs[LM_UARM_R], clip);

        Matrix_TranslateRel(bone[33], bone[34], bone[35]);
        Matrix_RotYXZsuperpack(&mesh_rots, 0);
        Output_InsertPolygons(g_Lara.mesh_ptrs[LM_LARM_R], clip);

        Matrix_TranslateRel(bone[37], bone[38], bone[39]);
        Matrix_RotYXZsuperpack(&mesh_rots, 0);
        Output_InsertPolygons(g_Lara.mesh_ptrs[LM_HAND_R], clip);

        if (g_Lara.right_arm.flash_gun) {
            saved_matrix = *g_MatrixPtr;
        }
        Matrix_Pop();

        Matrix_Push();
        Matrix_TranslateRel(bone[41], bone[42], bone[43]);
        g_MatrixPtr->_00 = g_MatrixPtr[-2]._00;
        g_MatrixPtr->_01 = g_MatrixPtr[-2]._01;
        g_MatrixPtr->_02 = g_MatrixPtr[-2]._02;
        g_MatrixPtr->_10 = g_MatrixPtr[-2]._10;
        g_MatrixPtr->_11 = g_MatrixPtr[-2]._11;
        g_MatrixPtr->_12 = g_MatrixPtr[-2]._12;
        g_MatrixPtr->_20 = g_MatrixPtr[-2]._20;
        g_MatrixPtr->_21 = g_MatrixPtr[-2]._21;
        g_MatrixPtr->_22 = g_MatrixPtr[-2]._22;
        Matrix_RotYXZ(
            g_Lara.left_arm.rot.y, g_Lara.left_arm.rot.x,
            g_Lara.left_arm.rot.z);
        mesh_rots = &g_Lara.left_arm.frame_base
                         [(g_Anims[g_Lara.left_arm.anim_num].interpolation >> 8)
                              * (g_Lara.left_arm.frame_num
                                 - g_Anims[g_Lara.left_arm.anim_num].frame_base)
                          + FBBOX_ROT];
        Matrix_RotYXZsuperpack(&mesh_rots, 11);
        Output_InsertPolygons(g_Lara.mesh_ptrs[LM_UARM_L], clip);

        Matrix_TranslateRel(bone[45], bone[46], bone[47]);
        Matrix_RotYXZsuperpack(&mesh_rots, 0);
        Output_InsertPolygons(g_Lara.mesh_ptrs[LM_LARM_L], clip);

        Matrix_TranslateRel(bone[49], bone[50], bone[51]);
        Matrix_RotYXZsuperpack(&mesh_rots, 0);
        Output_InsertPolygons(g_Lara.mesh_ptrs[LM_HAND_L], clip);

        if (g_Lara.left_arm.flash_gun) {
            Gun_DrawFlash(gun_type, clip);
        }

        if (g_Lara.right_arm.flash_gun) {
            *g_MatrixPtr = saved_matrix;
            Gun_DrawFlash(gun_type, clip);
        }
        Matrix_Pop();
        break;

    case LGT_SHOTGUN:
    case LGT_M16:
    case LGT_GRENADE:
    case LGT_HARPOON:
        Matrix_Push();
        Matrix_TranslateRel(bone[29], bone[30], bone[31]);
        mesh_rots =
            &g_Lara.right_arm.frame_base
                 [g_Lara.right_arm.frame_num
                      * (g_Anims[g_Lara.right_arm.anim_num].interpolation >> 8)
                  + FBBOX_ROT];
        Matrix_RotYXZsuperpack(&mesh_rots, 8);
        Output_InsertPolygons(g_Lara.mesh_ptrs[LM_UARM_R], clip);

        Matrix_TranslateRel(bone[33], bone[34], bone[35]);
        Matrix_RotYXZsuperpack(&mesh_rots, 0);
        Output_InsertPolygons(g_Lara.mesh_ptrs[LM_LARM_R], clip);

        Matrix_TranslateRel(bone[37], bone[38], bone[39]);
        Matrix_RotYXZsuperpack(&mesh_rots, 0);
        Output_InsertPolygons(g_Lara.mesh_ptrs[LM_HAND_R], clip);

        if (g_Lara.right_arm.flash_gun) {
            saved_matrix = *g_MatrixPtr;
        }
        Matrix_Pop();

        Matrix_Push();
        Matrix_TranslateRel(bone[41], bone[42], bone[43]);
        Matrix_RotYXZsuperpack(&mesh_rots, 0);
        Output_InsertPolygons(g_Lara.mesh_ptrs[LM_UARM_L], clip);

        Matrix_TranslateRel(bone[45], bone[46], bone[47]);
        Matrix_RotYXZsuperpack(&mesh_rots, 0);
        Output_InsertPolygons(g_Lara.mesh_ptrs[LM_LARM_L], clip);

        Matrix_TranslateRel(bone[49], bone[50], bone[51]);
        Matrix_RotYXZsuperpack(&mesh_rots, 0);
        Output_InsertPolygons(g_Lara.mesh_ptrs[LM_HAND_L], clip);

        if (g_Lara.right_arm.flash_gun) {
            *g_MatrixPtr = saved_matrix;
            Gun_DrawFlash(gun_type, clip);
        }

        Matrix_Pop();
        break;

    default:
        break;
    }

    Matrix_Pop();
    Matrix_Pop();

finish:
    g_PhdWinLeft = left;
    g_PhdWinRight = right;
    g_PhdWinTop = top;
    g_PhdWinBottom = bottom;
}

void __cdecl Lara_Draw_I(
    const ITEM *const item, const FRAME_INFO *const frame1,
    const FRAME_INFO *const frame2, const int32_t frac, const int32_t rate)
{
    const OBJECT *const object = &g_Objects[item->object_id];
    const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(item);

    if (g_Lara.skidoo == NO_ITEM) {
        S_PrintShadow(object->shadow_size, bounds, item);
    }

    MATRIX saved_matrix = *g_MatrixPtr;

    Matrix_Push();
    Matrix_TranslateAbs(item->pos.x, item->pos.y, item->pos.z);
    Matrix_RotYXZ(item->rot.y, item->rot.x, item->rot.z);

    const int32_t clip = S_GetObjectBounds(&frame1->bounds);

    if (!clip) {
        Matrix_Pop();
        return;
    }

    Matrix_Push();
    Output_CalculateObjectLighting(item, frame1);

    const int32_t *bone = &g_AnimBones[object->bone_idx];
    const int16_t *mesh_rots_1 = frame1->mesh_rots;
    const int16_t *mesh_rots_2 = frame2->mesh_rots;
    const int16_t *mesh_rots_1_c;
    const int16_t *mesh_rots_2_c;

    Matrix_InitInterpolate(frac, rate);
    Matrix_TranslateRel_ID(
        frame1->offset.x, frame1->offset.y, frame1->offset.z, frame2->offset.x,
        frame2->offset.y, frame2->offset.z);
    Matrix_RotYXZsuperpack_I(&mesh_rots_1, &mesh_rots_2, 0);
    Output_InsertPolygons_I(g_Lara.mesh_ptrs[LM_HIPS], clip);

    Matrix_Push_I();
    Matrix_TranslateRel_I(bone[1], bone[2], bone[3]);
    Matrix_RotYXZsuperpack_I(&mesh_rots_1, &mesh_rots_2, 0);
    Output_InsertPolygons_I(g_Lara.mesh_ptrs[LM_THIGH_L], clip);

    Matrix_TranslateRel_I(bone[5], bone[6], bone[7]);
    Matrix_RotYXZsuperpack_I(&mesh_rots_1, &mesh_rots_2, 0);
    Output_InsertPolygons_I(g_Lara.mesh_ptrs[LM_CALF_L], clip);

    Matrix_TranslateRel_I(bone[9], bone[10], bone[11]);
    Matrix_RotYXZsuperpack_I(&mesh_rots_1, &mesh_rots_2, 0);
    Output_InsertPolygons_I(g_Lara.mesh_ptrs[LM_FOOT_L], clip);
    Matrix_Pop_I();

    Matrix_Push_I();
    Matrix_TranslateRel_I(bone[13], bone[14], bone[15]);
    Matrix_RotYXZsuperpack_I(&mesh_rots_1, &mesh_rots_2, 0);
    Output_InsertPolygons_I(g_Lara.mesh_ptrs[LM_THIGH_R], clip);

    Matrix_TranslateRel_I(bone[17], bone[18], bone[19]);
    Matrix_RotYXZsuperpack_I(&mesh_rots_1, &mesh_rots_2, 0);
    Output_InsertPolygons_I(g_Lara.mesh_ptrs[LM_CALF_R], clip);

    Matrix_TranslateRel_I(bone[21], bone[22], bone[23]);
    Matrix_RotYXZsuperpack_I(&mesh_rots_1, &mesh_rots_2, 0);
    Output_InsertPolygons_I(g_Lara.mesh_ptrs[LM_FOOT_R], clip);
    Matrix_Pop_I();

    Matrix_TranslateRel_I(bone[25], bone[26], bone[27]);
    if (g_Lara.weapon_item != -1 && g_Lara.gun_type == 5
        && ((g_Items[g_Lara.weapon_item].current_anim_state) == 0
            || g_Items[g_Lara.weapon_item].current_anim_state == 2
            || g_Items[g_Lara.weapon_item].current_anim_state == 4)) {
        mesh_rots_2 =
            &g_Lara.right_arm.frame_base
                 [g_Lara.right_arm.frame_num
                      * (g_Anims[g_Lara.right_arm.anim_num].interpolation >> 8)
                  + FBBOX_ROT];
        mesh_rots_1 = mesh_rots_2;
        Matrix_RotYXZsuperpack_I(&mesh_rots_1, &mesh_rots_2, 7);
    } else {
        Matrix_RotYXZsuperpack_I(&mesh_rots_1, &mesh_rots_2, 0);
    }
    Matrix_RotYXZ_I(g_Lara.torso_y_rot, g_Lara.torso_x_rot, g_Lara.torso_z_rot);
    Output_InsertPolygons_I(g_Lara.mesh_ptrs[LM_TORSO], clip);

    Matrix_Push_I();
    Matrix_TranslateRel_I(bone[53], bone[54], bone[55]);
    mesh_rots_1_c = mesh_rots_1;
    mesh_rots_2_c = mesh_rots_2;
    Matrix_RotYXZsuperpack_I(&mesh_rots_1, &mesh_rots_2, 6);
    mesh_rots_1 = mesh_rots_1_c;
    mesh_rots_2 = mesh_rots_2_c;
    Matrix_RotYXZ_I(g_Lara.head_y_rot, g_Lara.head_x_rot, g_Lara.head_z_rot);
    Output_InsertPolygons_I(g_Lara.mesh_ptrs[LM_HEAD], clip);

    *g_MatrixPtr = saved_matrix;
    DrawHair();
    Matrix_Pop_I();

    if (g_Lara.back_gun) {
        Matrix_Push_I();
        const int32_t *bone_c =
            &g_AnimBones[g_Objects[g_Lara.back_gun].bone_idx];
        Matrix_TranslateRel_I(bone_c[53], bone_c[54], bone_c[55]);
        mesh_rots_1_c = g_Objects[g_Lara.back_gun].frame_base + FBBOX_ROT;
        mesh_rots_2_c = g_Objects[g_Lara.back_gun].frame_base + FBBOX_ROT;
        Matrix_RotYXZsuperpack_I(&mesh_rots_1_c, &mesh_rots_2_c, 14);
        Output_InsertPolygons_I(
            g_Meshes[g_Objects[g_Lara.back_gun].mesh_idx + LM_HEAD], clip);
        Matrix_Pop_I();
    }

    LARA_GUN_TYPE gun_type = LGT_UNARMED;
    if (g_Lara.gun_status == LGS_READY || g_Lara.gun_status == LGS_SPECIAL
        || g_Lara.gun_status == LGS_DRAW || g_Lara.gun_status == LGS_UNDRAW) {
        gun_type = g_Lara.gun_type;
    }

    switch (gun_type) {
    case LGT_UNARMED:
    case LGT_FLARE:
        Matrix_Push_I();
        Matrix_TranslateRel_I(bone[29], bone[30], bone[31]);
        Matrix_RotYXZsuperpack_I(&mesh_rots_1, &mesh_rots_2, 0);
        Output_InsertPolygons_I(g_Lara.mesh_ptrs[LM_UARM_R], clip);

        Matrix_TranslateRel_I(bone[33], bone[34], bone[35]);
        Matrix_RotYXZsuperpack_I(&mesh_rots_1, &mesh_rots_2, 0);
        Output_InsertPolygons_I(g_Lara.mesh_ptrs[LM_LARM_R], clip);

        Matrix_TranslateRel_I(bone[37], bone[38], bone[39]);
        Matrix_RotYXZsuperpack_I(&mesh_rots_1, &mesh_rots_2, 0);
        Output_InsertPolygons_I(g_Lara.mesh_ptrs[LM_HAND_R], clip);
        Matrix_Pop_I();

        Matrix_Push_I();
        Matrix_TranslateRel_I(bone[41], bone[42], bone[43]);
        if (g_Lara.flare_control_left) {
            mesh_rots_1 =
                &g_Lara.left_arm.frame_base
                     [(g_Anims[g_Lara.left_arm.anim_num].interpolation >> 8)
                          * (g_Lara.left_arm.frame_num
                             - g_Anims[g_Lara.left_arm.anim_num].frame_base)
                      + FBBOX_ROT];

            mesh_rots_2 =
                &g_Lara.left_arm.frame_base
                     [(g_Anims[g_Lara.left_arm.anim_num].interpolation >> 8)
                          * (g_Lara.left_arm.frame_num
                             - g_Anims[g_Lara.left_arm.anim_num].frame_base)
                      + FBBOX_ROT];
            Matrix_RotYXZsuperpack_I(&mesh_rots_1, &mesh_rots_2, 11);
        } else {
            Matrix_RotYXZsuperpack_I(&mesh_rots_1, &mesh_rots_2, 0);
        }
        Output_InsertPolygons_I(g_Lara.mesh_ptrs[LM_UARM_L], clip);

        Matrix_TranslateRel_I(bone[45], bone[46], bone[47]);
        Matrix_RotYXZsuperpack_I(&mesh_rots_1, &mesh_rots_2, 0);
        Output_InsertPolygons_I(g_Lara.mesh_ptrs[LM_LARM_L], clip);

        Matrix_TranslateRel_I(bone[49], bone[50], bone[51]);
        Matrix_RotYXZsuperpack_I(&mesh_rots_1, &mesh_rots_2, 0);
        Output_InsertPolygons_I(g_Lara.mesh_ptrs[LM_HAND_L], clip);

        if (g_Lara.gun_type == LGT_FLARE && g_Lara.left_arm.flash_gun) {
            Matrix_TranslateRel_I(11, 32, 80);
            Matrix_RotX_I(-16380);
            Matrix_RotY_I(2 * Random_GetDraw());
            S_CalculateStaticLight(2048);
            Output_InsertPolygons_I(g_Meshes[g_Objects[235].mesh_idx], clip);
        }
        Matrix_Pop();
        break;

    case LGT_PISTOLS:
    case LGT_MAGNUMS:
    case LGT_UZIS:
        Matrix_Push_I();
        Matrix_TranslateRel_I(bone[29], bone[30], bone[31]);
        Matrix_InterpolateArm();
        Matrix_RotYXZ(
            g_Lara.right_arm.rot.y, g_Lara.right_arm.rot.x,
            g_Lara.right_arm.rot.z);
        mesh_rots_1 =
            &g_Lara.right_arm.frame_base
                 [(g_Anims[g_Lara.right_arm.anim_num].interpolation >> 8)
                      * (g_Lara.right_arm.frame_num
                         - g_Anims[g_Lara.right_arm.anim_num].frame_base)
                  + FBBOX_ROT];
        Matrix_RotYXZsuperpack(&mesh_rots_1, 8);
        Output_InsertPolygons(g_Lara.mesh_ptrs[LM_UARM_R], clip);

        Matrix_TranslateRel(bone[33], bone[34], bone[35]);
        Matrix_RotYXZsuperpack(&mesh_rots_1, 0);
        Output_InsertPolygons(g_Lara.mesh_ptrs[LM_LARM_R], clip);

        Matrix_TranslateRel(bone[37], bone[38], bone[39]);
        Matrix_RotYXZsuperpack(&mesh_rots_1, 0);
        Output_InsertPolygons(g_Lara.mesh_ptrs[LM_HAND_R], clip);

        if (g_Lara.right_arm.flash_gun) {
            saved_matrix = *g_MatrixPtr;
        }
        Matrix_Pop_I();

        Matrix_Push_I();
        Matrix_TranslateRel_I(bone[41], bone[42], bone[43]);
        Matrix_InterpolateArm();
        Matrix_RotYXZ(
            g_Lara.left_arm.rot.y, g_Lara.left_arm.rot.x,
            g_Lara.left_arm.rot.z);
        mesh_rots_1 =
            &g_Lara.left_arm.frame_base
                 [(g_Anims[g_Lara.left_arm.anim_num].interpolation >> 8)
                      * (g_Lara.left_arm.frame_num
                         - g_Anims[g_Lara.left_arm.anim_num].frame_base)
                  + FBBOX_ROT];
        Matrix_RotYXZsuperpack(&mesh_rots_1, 11);
        Output_InsertPolygons(g_Lara.mesh_ptrs[LM_UARM_L], clip);

        Matrix_TranslateRel(bone[45], bone[46], bone[47]);
        Matrix_RotYXZsuperpack(&mesh_rots_1, 0);
        Output_InsertPolygons(g_Lara.mesh_ptrs[LM_LARM_L], clip);

        Matrix_TranslateRel(bone[49], bone[50], bone[51]);
        Matrix_RotYXZsuperpack(&mesh_rots_1, 0);
        Output_InsertPolygons(g_Lara.mesh_ptrs[LM_HAND_L], clip);

        if (g_Lara.left_arm.flash_gun) {
            Gun_DrawFlash((int32_t)gun_type, clip);
        }

        if (g_Lara.right_arm.flash_gun) {
            *g_MatrixPtr = saved_matrix;
            Gun_DrawFlash(gun_type, clip);
        }
        Matrix_Pop();
        break;

    case LGT_SHOTGUN:
    case LGT_M16:
    case LGT_GRENADE:
    case LGT_HARPOON:
        Matrix_Push_I();
        Matrix_TranslateRel_I(bone[29], bone[30], bone[31]);
        mesh_rots_1 =
            &g_Lara.right_arm.frame_base
                 [g_Lara.right_arm.frame_num
                      * (g_Anims[g_Lara.right_arm.anim_num].interpolation >> 8)
                  + FBBOX_ROT];
        mesh_rots_2 =
            &g_Lara.right_arm.frame_base
                 [g_Lara.right_arm.frame_num
                      * (g_Anims[g_Lara.right_arm.anim_num].interpolation >> 8)
                  + FBBOX_ROT];
        Matrix_RotYXZsuperpack_I(&mesh_rots_1, &mesh_rots_2, 8);
        Output_InsertPolygons_I(g_Lara.mesh_ptrs[LM_UARM_R], clip);

        Matrix_TranslateRel_I(bone[33], bone[34], bone[35]);
        Matrix_RotYXZsuperpack_I(&mesh_rots_1, &mesh_rots_2, 0);
        Output_InsertPolygons_I(g_Lara.mesh_ptrs[LM_LARM_R], clip);

        Matrix_TranslateRel_I(bone[37], bone[38], bone[39]);
        Matrix_RotYXZsuperpack_I(&mesh_rots_1, &mesh_rots_2, 0);
        Output_InsertPolygons_I(g_Lara.mesh_ptrs[LM_HAND_R], clip);

        if (g_Lara.right_arm.flash_gun) {
            saved_matrix = *g_MatrixPtr;
        }
        Matrix_Pop_I();

        Matrix_Push_I();
        Matrix_TranslateRel_I(bone[41], bone[42], bone[43]);
        Matrix_RotYXZsuperpack_I(&mesh_rots_1, &mesh_rots_2, 0);
        Output_InsertPolygons_I(g_Lara.mesh_ptrs[LM_UARM_L], clip);

        Matrix_TranslateRel_I(bone[45], bone[46], bone[47]);
        Matrix_RotYXZsuperpack_I(&mesh_rots_1, &mesh_rots_2, 0);
        Output_InsertPolygons_I(g_Lara.mesh_ptrs[LM_LARM_L], clip);

        Matrix_TranslateRel_I(bone[49], bone[50], bone[51]);
        Matrix_RotYXZsuperpack_I(&mesh_rots_1, &mesh_rots_2, 0);
        Output_InsertPolygons_I(g_Lara.mesh_ptrs[LM_HAND_L], clip);

        if (g_Lara.right_arm.flash_gun) {
            *g_MatrixPtr = saved_matrix;
            Gun_DrawFlash(gun_type, clip);
        }

        Matrix_Pop();
        break;

    default:
        break;
    }

    Matrix_Pop();
    Matrix_Pop();
}
