#include "game/lara/lara_draw.h"

#include "3dsystem/matrix.h"
#include "game/draw.h"
#include "game/hair.h"
#include "game/output.h"
#include "game/viewport.h"
#include "global/vars.h"
#include "specific/s_misc.h"

void Lara_Draw(ITEM_INFO *item)
{
    OBJECT_INFO *object;
    int16_t *frame;
    int16_t *frmptr[2];
    PHD_MATRIX saved_matrix;

    int32_t top = g_PhdTop;
    int32_t left = g_PhdLeft;
    int32_t bottom = g_PhdBottom;
    int32_t right = g_PhdRight;

    g_PhdLeft = ViewPort_GetMinX();
    g_PhdTop = ViewPort_GetMinY();
    g_PhdBottom = ViewPort_GetMaxY();
    g_PhdRight = ViewPort_GetMaxX();

    if (g_Lara.hit_direction < 0) {
        int32_t rate;
        int32_t frac = GetFrames(item, frmptr, &rate);
        if (frac) {
            Lara_Draw_I(item, frmptr[0], frmptr[1], frac, rate);
            g_PhdLeft = left;
            g_PhdRight = right;
            g_PhdTop = top;
            g_PhdBottom = bottom;
            return;
        }
    }

    object = &g_Objects[item->object_number];
    if (g_Lara.hit_direction >= 0) {
        switch (g_Lara.hit_direction) {
        default:
        case DIR_NORTH:
            frame = g_Anims[LA_SPAZ_FORWARD].frame_ptr;
            break;
        case DIR_EAST:
            frame = g_Anims[LA_SPAZ_RIGHT].frame_ptr;
            break;
        case DIR_SOUTH:
            frame = g_Anims[LA_SPAZ_BACK].frame_ptr;
            break;
        case DIR_WEST:
            frame = g_Anims[LA_SPAZ_LEFT].frame_ptr;
            break;
        }

        frame += g_Lara.hit_frame * (object->nmeshes * 2 + FRAME_ROT);
    } else {
        frame = frmptr[0];
    }

    // save matrix for hair
    saved_matrix = *g_PhdMatrixPtr;

    Output_DrawShadow(object->shadow_size, frame, item);
    phd_PushMatrix();
    phd_TranslateAbs(item->pos.x, item->pos.y, item->pos.z);
    phd_RotYXZ(item->pos.y_rot, item->pos.x_rot, item->pos.z_rot);

    int32_t clip = S_GetObjectBounds(frame);
    if (!clip) {
        phd_PopMatrix();
        return;
    }

    phd_PushMatrix();

    CalculateObjectLighting(item, frame);

    int32_t *bone = &g_AnimBones[object->bone_index];
    int32_t *packed_rotation = (int32_t *)(frame + FRAME_ROT);

    phd_TranslateRel(
        frame[FRAME_POS_X], frame[FRAME_POS_Y], frame[FRAME_POS_Z]);
    phd_RotYXZpack(packed_rotation[LM_HIPS]);
    Output_DrawPolygons(g_Lara.mesh_ptrs[LM_HIPS], clip);

    phd_PushMatrix();

    phd_TranslateRel(bone[1], bone[2], bone[3]);
    phd_RotYXZpack(packed_rotation[LM_THIGH_L]);
    Output_DrawPolygons(g_Lara.mesh_ptrs[LM_THIGH_L], clip);

    phd_TranslateRel(bone[5], bone[6], bone[7]);
    phd_RotYXZpack(packed_rotation[LM_CALF_L]);
    Output_DrawPolygons(g_Lara.mesh_ptrs[LM_CALF_L], clip);

    phd_TranslateRel(bone[9], bone[10], bone[11]);
    phd_RotYXZpack(packed_rotation[LM_FOOT_L]);
    Output_DrawPolygons(g_Lara.mesh_ptrs[LM_FOOT_L], clip);

    phd_PopMatrix();

    phd_PushMatrix();

    phd_TranslateRel(bone[13], bone[14], bone[15]);
    phd_RotYXZpack(packed_rotation[LM_THIGH_R]);
    Output_DrawPolygons(g_Lara.mesh_ptrs[LM_THIGH_R], clip);

    phd_TranslateRel(bone[17], bone[18], bone[19]);
    phd_RotYXZpack(packed_rotation[LM_CALF_R]);
    Output_DrawPolygons(g_Lara.mesh_ptrs[LM_CALF_R], clip);

    phd_TranslateRel(bone[21], bone[22], bone[23]);
    phd_RotYXZpack(packed_rotation[LM_FOOT_R]);
    Output_DrawPolygons(g_Lara.mesh_ptrs[LM_FOOT_R], clip);

    phd_PopMatrix();

    phd_TranslateRel(bone[25], bone[26], bone[27]);
    phd_RotYXZpack(packed_rotation[LM_TORSO]);
    phd_RotYXZ(g_Lara.torso_y_rot, g_Lara.torso_x_rot, g_Lara.torso_z_rot);
    Output_DrawPolygons(g_Lara.mesh_ptrs[LM_TORSO], clip);

    phd_PushMatrix();

    phd_TranslateRel(bone[53], bone[54], bone[55]);
    phd_RotYXZpack(packed_rotation[LM_HEAD]);
    phd_RotYXZ(g_Lara.head_y_rot, g_Lara.head_x_rot, g_Lara.head_z_rot);
    Output_DrawPolygons(g_Lara.mesh_ptrs[LM_HEAD], clip);

    *g_PhdMatrixPtr = saved_matrix;
    DrawHair();

    phd_PopMatrix();

    int32_t fire_arms = 0;
    if (g_Lara.gun_status == LGS_READY || g_Lara.gun_status == LGS_DRAW
        || g_Lara.gun_status == LGS_UNDRAW) {
        fire_arms = g_Lara.gun_type;
    }

    switch (fire_arms) {
    case LGT_UNARMED:
        phd_PushMatrix();

        phd_TranslateRel(bone[29], bone[30], bone[31]);
        phd_RotYXZpack(packed_rotation[LM_UARM_R]);
        Output_DrawPolygons(g_Lara.mesh_ptrs[LM_UARM_R], clip);

        phd_TranslateRel(bone[33], bone[34], bone[35]);
        phd_RotYXZpack(packed_rotation[LM_LARM_R]);
        Output_DrawPolygons(g_Lara.mesh_ptrs[LM_LARM_R], clip);

        phd_TranslateRel(bone[37], bone[38], bone[39]);
        phd_RotYXZpack(packed_rotation[LM_HAND_R]);
        Output_DrawPolygons(g_Lara.mesh_ptrs[LM_HAND_R], clip);

        phd_PopMatrix();

        phd_PushMatrix();

        phd_TranslateRel(bone[41], bone[42], bone[43]);
        phd_RotYXZpack(packed_rotation[LM_UARM_L]);
        Output_DrawPolygons(g_Lara.mesh_ptrs[11], clip);

        phd_TranslateRel(bone[45], bone[46], bone[47]);
        phd_RotYXZpack(packed_rotation[LM_LARM_L]);
        Output_DrawPolygons(g_Lara.mesh_ptrs[LM_LARM_L], clip);

        phd_TranslateRel(bone[49], bone[50], bone[51]);
        phd_RotYXZpack(packed_rotation[LM_HAND_L]);
        Output_DrawPolygons(g_Lara.mesh_ptrs[LM_HAND_L], clip);

        phd_PopMatrix();
        break;

    case LGT_PISTOLS:
    case LGT_MAGNUMS:
    case LGT_UZIS:
        phd_PushMatrix();

        phd_TranslateRel(bone[29], bone[30], bone[31]);

        g_PhdMatrixPtr->_00 = g_PhdMatrixPtr[-2]._00;
        g_PhdMatrixPtr->_01 = g_PhdMatrixPtr[-2]._01;
        g_PhdMatrixPtr->_02 = g_PhdMatrixPtr[-2]._02;
        g_PhdMatrixPtr->_10 = g_PhdMatrixPtr[-2]._10;
        g_PhdMatrixPtr->_11 = g_PhdMatrixPtr[-2]._11;
        g_PhdMatrixPtr->_12 = g_PhdMatrixPtr[-2]._12;
        g_PhdMatrixPtr->_20 = g_PhdMatrixPtr[-2]._20;
        g_PhdMatrixPtr->_21 = g_PhdMatrixPtr[-2]._21;
        g_PhdMatrixPtr->_22 = g_PhdMatrixPtr[-2]._22;

        packed_rotation =
            (int32_t
                 *)(g_Lara.right_arm.frame_base + g_Lara.right_arm.frame_number * (object->nmeshes * 2 + FRAME_ROT) + 10);
        phd_RotYXZ(
            g_Lara.right_arm.y_rot, g_Lara.right_arm.x_rot,
            g_Lara.right_arm.z_rot);
        phd_RotYXZpack(packed_rotation[LM_UARM_R]);
        Output_DrawPolygons(g_Lara.mesh_ptrs[LM_UARM_R], clip);

        phd_TranslateRel(bone[33], bone[34], bone[35]);
        phd_RotYXZpack(packed_rotation[LM_LARM_R]);
        Output_DrawPolygons(g_Lara.mesh_ptrs[LM_LARM_R], clip);

        phd_TranslateRel(bone[37], bone[38], bone[39]);
        phd_RotYXZpack(packed_rotation[LM_HAND_R]);
        Output_DrawPolygons(g_Lara.mesh_ptrs[LM_HAND_R], clip);

        if (g_Lara.right_arm.flash_gun) {
            saved_matrix = *g_PhdMatrixPtr;
        }

        phd_PopMatrix();

        phd_PushMatrix();

        phd_TranslateRel(bone[41], bone[42], bone[43]);

        g_PhdMatrixPtr->_00 = g_PhdMatrixPtr[-2]._00;
        g_PhdMatrixPtr->_01 = g_PhdMatrixPtr[-2]._01;
        g_PhdMatrixPtr->_02 = g_PhdMatrixPtr[-2]._02;
        g_PhdMatrixPtr->_10 = g_PhdMatrixPtr[-2]._10;
        g_PhdMatrixPtr->_11 = g_PhdMatrixPtr[-2]._11;
        g_PhdMatrixPtr->_12 = g_PhdMatrixPtr[-2]._12;
        g_PhdMatrixPtr->_20 = g_PhdMatrixPtr[-2]._20;
        g_PhdMatrixPtr->_21 = g_PhdMatrixPtr[-2]._21;
        g_PhdMatrixPtr->_22 = g_PhdMatrixPtr[-2]._22;

        packed_rotation =
            (int32_t
                 *)(g_Lara.left_arm.frame_base + g_Lara.left_arm.frame_number * (object->nmeshes * 2 + FRAME_ROT) + 10);
        phd_RotYXZ(
            g_Lara.left_arm.y_rot, g_Lara.left_arm.x_rot,
            g_Lara.left_arm.z_rot);
        phd_RotYXZpack(packed_rotation[LM_UARM_L]);
        Output_DrawPolygons(g_Lara.mesh_ptrs[LM_UARM_L], clip);

        phd_TranslateRel(bone[45], bone[46], bone[47]);
        phd_RotYXZpack(packed_rotation[LM_LARM_L]);
        Output_DrawPolygons(g_Lara.mesh_ptrs[LM_LARM_L], clip);

        phd_TranslateRel(bone[49], bone[50], bone[51]);
        phd_RotYXZpack(packed_rotation[LM_HAND_L]);
        Output_DrawPolygons(g_Lara.mesh_ptrs[LM_HAND_L], clip);

        if (g_Lara.left_arm.flash_gun) {
            DrawGunFlash(fire_arms, clip);
        }
        if (g_Lara.right_arm.flash_gun) {
            *g_PhdMatrixPtr = saved_matrix;
            DrawGunFlash(fire_arms, clip);
        }

        phd_PopMatrix();
        break;

    case LGT_SHOTGUN:
        phd_PushMatrix();

        packed_rotation =
            (int32_t
                 *)(g_Lara.right_arm.frame_base + g_Lara.right_arm.frame_number * (object->nmeshes * 2 + FRAME_ROT) + 10);
        phd_TranslateRel(bone[29], bone[30], bone[31]);
        phd_RotYXZpack(packed_rotation[LM_UARM_R]);
        Output_DrawPolygons(g_Lara.mesh_ptrs[LM_UARM_R], clip);

        phd_TranslateRel(bone[33], bone[34], bone[35]);
        phd_RotYXZpack(packed_rotation[LM_LARM_R]);
        Output_DrawPolygons(g_Lara.mesh_ptrs[LM_LARM_R], clip);

        phd_TranslateRel(bone[37], bone[38], bone[39]);
        phd_RotYXZpack(packed_rotation[LM_HAND_R]);
        Output_DrawPolygons(g_Lara.mesh_ptrs[LM_HAND_R], clip);

        if (g_Lara.right_arm.flash_gun) {
            saved_matrix = *g_PhdMatrixPtr;
        }

        phd_PopMatrix();

        phd_PushMatrix();

        packed_rotation =
            (int32_t
                 *)(g_Lara.left_arm.frame_base + g_Lara.left_arm.frame_number * (object->nmeshes * 2 + FRAME_ROT) + 10);
        phd_TranslateRel(bone[41], bone[42], bone[43]);
        phd_RotYXZpack(packed_rotation[LM_UARM_L]);
        Output_DrawPolygons(g_Lara.mesh_ptrs[LM_UARM_L], clip);

        phd_TranslateRel(bone[45], bone[46], bone[47]);
        phd_RotYXZpack(packed_rotation[LM_LARM_L]);
        Output_DrawPolygons(g_Lara.mesh_ptrs[LM_LARM_L], clip);

        phd_TranslateRel(bone[49], bone[50], bone[51]);
        phd_RotYXZpack(packed_rotation[LM_HAND_L]);
        Output_DrawPolygons(g_Lara.mesh_ptrs[LM_HAND_L], clip);

        if (g_Lara.right_arm.flash_gun) {
            *g_PhdMatrixPtr = saved_matrix;
            DrawGunFlash(fire_arms, clip);
        }

        phd_PopMatrix();
        break;
    }

    phd_PopMatrix();
    phd_PopMatrix();
    g_PhdLeft = left;
    g_PhdRight = right;
    g_PhdTop = top;
    g_PhdBottom = bottom;
}

void Lara_Draw_I(
    ITEM_INFO *item, int16_t *frame1, int16_t *frame2, int32_t frac,
    int32_t rate)
{
    PHD_MATRIX saved_matrix;

    OBJECT_INFO *object = &g_Objects[item->object_number];
    int16_t *bounds = GetBoundsAccurate(item);

    saved_matrix = *g_PhdMatrixPtr;

    Output_DrawShadow(object->shadow_size, bounds, item);
    phd_PushMatrix();
    phd_TranslateAbs(item->pos.x, item->pos.y, item->pos.z);
    phd_RotYXZ(item->pos.y_rot, item->pos.x_rot, item->pos.z_rot);

    int32_t clip = S_GetObjectBounds(frame1);
    if (!clip) {
        phd_PopMatrix();
        return;
    }

    phd_PushMatrix();

    CalculateObjectLighting(item, frame1);

    int32_t *bone = &g_AnimBones[object->bone_index];
    int32_t *packed_rotation1 = (int32_t *)(frame1 + FRAME_ROT);
    int32_t *packed_rotation2 = (int32_t *)(frame2 + FRAME_ROT);

    InitInterpolate(frac, rate);

    phd_TranslateRel_ID(
        frame1[FRAME_POS_X], frame1[FRAME_POS_Y], frame1[FRAME_POS_Z],
        frame2[FRAME_POS_X], frame2[FRAME_POS_Y], frame2[FRAME_POS_Z]);

    phd_RotYXZpack_I(packed_rotation1[LM_HIPS], packed_rotation2[LM_HIPS]);
    Output_DrawPolygons_I(g_Lara.mesh_ptrs[LM_HIPS], clip);

    phd_PushMatrix_I();

    phd_TranslateRel_I(bone[1], bone[2], bone[3]);
    phd_RotYXZpack_I(
        packed_rotation1[LM_THIGH_L], packed_rotation2[LM_THIGH_L]);
    Output_DrawPolygons_I(g_Lara.mesh_ptrs[LM_THIGH_L], clip);

    phd_TranslateRel_I(bone[5], bone[6], bone[7]);
    phd_RotYXZpack_I(packed_rotation1[LM_CALF_L], packed_rotation2[LM_CALF_L]);
    Output_DrawPolygons_I(g_Lara.mesh_ptrs[LM_CALF_L], clip);

    phd_TranslateRel_I(bone[9], bone[10], bone[11]);
    phd_RotYXZpack_I(packed_rotation1[LM_FOOT_L], packed_rotation2[LM_FOOT_L]);
    Output_DrawPolygons_I(g_Lara.mesh_ptrs[LM_FOOT_L], clip);

    phd_PopMatrix_I();

    phd_PushMatrix_I();

    phd_TranslateRel_I(bone[13], bone[14], bone[15]);
    phd_RotYXZpack_I(
        packed_rotation1[LM_THIGH_R], packed_rotation2[LM_THIGH_R]);
    Output_DrawPolygons_I(g_Lara.mesh_ptrs[LM_THIGH_R], clip);

    phd_TranslateRel_I(bone[17], bone[18], bone[19]);
    phd_RotYXZpack_I(packed_rotation1[LM_CALF_R], packed_rotation2[LM_CALF_R]);
    Output_DrawPolygons_I(g_Lara.mesh_ptrs[LM_CALF_R], clip);

    phd_TranslateRel_I(bone[21], bone[22], bone[23]);
    phd_RotYXZpack_I(packed_rotation1[LM_FOOT_R], packed_rotation2[LM_FOOT_R]);
    Output_DrawPolygons_I(g_Lara.mesh_ptrs[LM_FOOT_R], clip);

    phd_PopMatrix_I();

    phd_TranslateRel_I(bone[25], bone[26], bone[27]);
    phd_RotYXZpack_I(packed_rotation1[LM_TORSO], packed_rotation2[LM_TORSO]);
    phd_RotYXZ_I(g_Lara.torso_y_rot, g_Lara.torso_x_rot, g_Lara.torso_z_rot);
    Output_DrawPolygons_I(g_Lara.mesh_ptrs[LM_TORSO], clip);

    phd_PushMatrix_I();

    phd_TranslateRel_I(bone[53], bone[54], bone[55]);
    phd_RotYXZpack_I(packed_rotation1[LM_HEAD], packed_rotation2[LM_HEAD]);
    phd_RotYXZ_I(g_Lara.head_y_rot, g_Lara.head_x_rot, g_Lara.head_z_rot);
    Output_DrawPolygons_I(g_Lara.mesh_ptrs[LM_HEAD], clip);

    *g_PhdMatrixPtr = saved_matrix;
    DrawHair();

    phd_PopMatrix_I();

    int32_t fire_arms = 0;
    if (g_Lara.gun_status == LGS_READY || g_Lara.gun_status == LGS_DRAW
        || g_Lara.gun_status == LGS_UNDRAW) {
        fire_arms = g_Lara.gun_type;
    }

    switch (fire_arms) {
    case LGT_UNARMED:
        phd_PushMatrix_I();

        phd_TranslateRel_I(bone[29], bone[30], bone[31]);
        phd_RotYXZpack_I(
            packed_rotation1[LM_UARM_R], packed_rotation2[LM_UARM_R]);
        Output_DrawPolygons_I(g_Lara.mesh_ptrs[LM_UARM_R], clip);

        phd_TranslateRel_I(bone[33], bone[34], bone[35]);
        phd_RotYXZpack_I(
            packed_rotation1[LM_LARM_R], packed_rotation2[LM_LARM_R]);
        Output_DrawPolygons_I(g_Lara.mesh_ptrs[LM_LARM_R], clip);

        phd_TranslateRel_I(bone[37], bone[38], bone[39]);
        phd_RotYXZpack_I(
            packed_rotation1[LM_HAND_R], packed_rotation2[LM_HAND_R]);
        Output_DrawPolygons_I(g_Lara.mesh_ptrs[LM_HAND_R], clip);

        phd_PopMatrix_I();

        phd_PushMatrix_I();

        phd_TranslateRel_I(bone[41], bone[42], bone[43]);
        phd_RotYXZpack_I(
            packed_rotation1[LM_UARM_L], packed_rotation2[LM_UARM_L]);
        Output_DrawPolygons_I(g_Lara.mesh_ptrs[11], clip);

        phd_TranslateRel_I(bone[45], bone[46], bone[47]);
        phd_RotYXZpack_I(
            packed_rotation1[LM_LARM_L], packed_rotation2[LM_LARM_L]);
        Output_DrawPolygons_I(g_Lara.mesh_ptrs[LM_LARM_L], clip);

        phd_TranslateRel_I(bone[49], bone[50], bone[51]);
        phd_RotYXZpack_I(
            packed_rotation1[LM_HAND_L], packed_rotation2[LM_HAND_L]);
        Output_DrawPolygons_I(g_Lara.mesh_ptrs[LM_HAND_L], clip);

        phd_PopMatrix_I();
        break;

    case LGT_PISTOLS:
    case LGT_MAGNUMS:
    case LGT_UZIS:
        phd_PushMatrix_I();

        phd_TranslateRel_I(bone[29], bone[30], bone[31]);
        InterpolateArmMatrix();

        packed_rotation1 =
            (int32_t
                 *)(g_Lara.right_arm.frame_base + g_Lara.right_arm.frame_number * (object->nmeshes * 2 + FRAME_ROT) + 10);
        phd_RotYXZ(
            g_Lara.right_arm.y_rot, g_Lara.right_arm.x_rot,
            g_Lara.right_arm.z_rot);
        phd_RotYXZpack(packed_rotation1[LM_UARM_R]);
        Output_DrawPolygons(g_Lara.mesh_ptrs[LM_UARM_R], clip);

        phd_TranslateRel(bone[33], bone[34], bone[35]);
        phd_RotYXZpack(packed_rotation1[LM_LARM_R]);
        Output_DrawPolygons(g_Lara.mesh_ptrs[LM_LARM_R], clip);

        phd_TranslateRel(bone[37], bone[38], bone[39]);
        phd_RotYXZpack(packed_rotation1[LM_HAND_R]);
        Output_DrawPolygons(g_Lara.mesh_ptrs[LM_HAND_R], clip);

        if (g_Lara.right_arm.flash_gun) {
            saved_matrix = *g_PhdMatrixPtr;
        }

        phd_PopMatrix_I();

        phd_PushMatrix_I();

        phd_TranslateRel_I(bone[41], bone[42], bone[43]);
        InterpolateArmMatrix();

        packed_rotation1 =
            (int32_t
                 *)(g_Lara.left_arm.frame_base + g_Lara.left_arm.frame_number * (object->nmeshes * 2 + FRAME_ROT) + 10);
        phd_RotYXZ(
            g_Lara.left_arm.y_rot, g_Lara.left_arm.x_rot,
            g_Lara.left_arm.z_rot);
        phd_RotYXZpack(packed_rotation1[LM_UARM_L]);
        Output_DrawPolygons(g_Lara.mesh_ptrs[LM_UARM_L], clip);

        phd_TranslateRel(bone[45], bone[46], bone[47]);
        phd_RotYXZpack(packed_rotation1[LM_LARM_L]);
        Output_DrawPolygons(g_Lara.mesh_ptrs[LM_LARM_L], clip);

        phd_TranslateRel(bone[49], bone[50], bone[51]);
        phd_RotYXZpack(packed_rotation1[LM_HAND_L]);
        Output_DrawPolygons(g_Lara.mesh_ptrs[LM_HAND_L], clip);

        if (g_Lara.left_arm.flash_gun) {
            DrawGunFlash(fire_arms, clip);
        }

        if (g_Lara.right_arm.flash_gun) {
            *g_PhdMatrixPtr = saved_matrix;
            DrawGunFlash(fire_arms, clip);
        }

        phd_PopMatrix_I();
        break;

    case LGT_SHOTGUN:
        phd_PushMatrix_I();
        InterpolateMatrix();

        packed_rotation1 =
            (int32_t
                 *)(g_Lara.right_arm.frame_base + g_Lara.right_arm.frame_number * (object->nmeshes * 2 + FRAME_ROT) + 10);
        phd_TranslateRel(bone[29], bone[30], bone[31]);
        phd_RotYXZpack(packed_rotation1[LM_UARM_R]);
        Output_DrawPolygons(g_Lara.mesh_ptrs[LM_UARM_R], clip);

        phd_TranslateRel(bone[33], bone[34], bone[35]);
        phd_RotYXZpack(packed_rotation1[LM_LARM_R]);
        Output_DrawPolygons(g_Lara.mesh_ptrs[LM_LARM_R], clip);

        phd_TranslateRel(bone[37], bone[38], bone[39]);
        phd_RotYXZpack(packed_rotation1[LM_HAND_R]);
        Output_DrawPolygons(g_Lara.mesh_ptrs[LM_HAND_R], clip);

        if (g_Lara.right_arm.flash_gun) {
            saved_matrix = *g_PhdMatrixPtr;
        }

        phd_PopMatrix();

        phd_PushMatrix();

        packed_rotation1 =
            (int32_t
                 *)(g_Lara.left_arm.frame_base + g_Lara.left_arm.frame_number * (object->nmeshes * 2 + FRAME_ROT) + 10);
        phd_TranslateRel(bone[41], bone[42], bone[43]);
        phd_RotYXZpack(packed_rotation1[LM_UARM_L]);
        Output_DrawPolygons(g_Lara.mesh_ptrs[LM_UARM_L], clip);

        phd_TranslateRel(bone[45], bone[46], bone[47]);
        phd_RotYXZpack(packed_rotation1[LM_LARM_L]);
        Output_DrawPolygons(g_Lara.mesh_ptrs[LM_LARM_L], clip);

        phd_TranslateRel(bone[49], bone[50], bone[51]);
        phd_RotYXZpack(packed_rotation1[LM_HAND_L]);
        Output_DrawPolygons(g_Lara.mesh_ptrs[LM_HAND_L], clip);

        if (g_Lara.right_arm.flash_gun) {
            *g_PhdMatrixPtr = saved_matrix;
            DrawGunFlash(fire_arms, clip);
        }

        phd_PopMatrix_I();
        break;
    }

    phd_PopMatrix();
    phd_PopMatrix();
}
