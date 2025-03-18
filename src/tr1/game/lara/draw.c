#include "game/lara/draw.h"

#include "game/gun.h"
#include "game/items.h"
#include "game/lara/common.h"
#include "game/lara/hair.h"
#include "game/output.h"
#include "game/viewport.h"
#include "global/vars.h"

#include <libtrx/game/matrix.h>

static void M_DrawMesh(LARA_MESH mesh_idx, int32_t clip, bool interpolated);

static void M_DrawMesh(
    const LARA_MESH mesh_idx, const int32_t clip, const bool interpolated)
{
    const OBJECT_MESH *const mesh = Lara_GetMesh(mesh_idx);
    if (interpolated) {
        Output_DrawObjectMesh_I(mesh, clip);
    } else {
        Output_DrawObjectMesh(mesh, clip);
    }
}

void Lara_Draw(const ITEM *const item)
{
    ANIM_FRAME *frmptr[2];
    MATRIX saved_matrix;

    int32_t top = g_PhdTop;
    int32_t left = g_PhdLeft;
    int32_t bottom = g_PhdBottom;
    int32_t right = g_PhdRight;

    if (g_LaraItem->flags & IF_INVISIBLE) {
        return;
    }

    g_PhdLeft = Viewport_GetMinX();
    g_PhdTop = Viewport_GetMinY();
    g_PhdBottom = Viewport_GetMaxY();
    g_PhdRight = Viewport_GetMaxX();

    if (g_Lara.hit_direction < 0) {
        int32_t rate;
        int32_t frac = Item_GetFrames(item, frmptr, &rate);
        if (frac) {
            Lara_Draw_I(item, frmptr[0], frmptr[1], frac, rate);
            goto end;
        }
    }

    const ANIM_FRAME *const hit_frame = Lara_GetHitFrame(item);
    const ANIM_FRAME *const frame =
        hit_frame == nullptr ? frmptr[0] : hit_frame;

    // save matrix for hair
    saved_matrix = *g_MatrixPtr;

    const OBJECT *const obj = Object_Get(item->object_id);
    Output_DrawShadow(obj->shadow_size, &frame->bounds, item);

    Matrix_Push();
    Matrix_TranslateAbs32(item->interp.result.pos);
    Matrix_Rot16(item->interp.result.rot);

    int32_t clip = Output_GetObjectBounds(&frame->bounds);
    if (!clip) {
        Matrix_Pop();
        return;
    }

    Matrix_Push();

    Output_CalculateObjectLighting(item, &frame->bounds);

    const ANIM_BONE *const bone = Object_GetBone(obj, 0);
    const XYZ_16 *mesh_rots = frame->mesh_rots;

    Matrix_TranslateRel16(frame->offset);
    Matrix_Rot16(mesh_rots[LM_HIPS]);
    M_DrawMesh(LM_HIPS, clip, false);

    Matrix_Push();

    Matrix_TranslateRel32(bone[0].pos);
    Matrix_Rot16(mesh_rots[LM_THIGH_L]);
    M_DrawMesh(LM_THIGH_L, clip, false);

    Matrix_TranslateRel32(bone[1].pos);
    Matrix_Rot16(mesh_rots[LM_CALF_L]);
    M_DrawMesh(LM_CALF_L, clip, false);

    Matrix_TranslateRel32(bone[2].pos);
    Matrix_Rot16(mesh_rots[LM_FOOT_L]);
    M_DrawMesh(LM_FOOT_L, clip, false);

    Matrix_Pop();

    Matrix_Push();

    Matrix_TranslateRel32(bone[3].pos);
    Matrix_Rot16(mesh_rots[LM_THIGH_R]);
    M_DrawMesh(LM_THIGH_R, clip, false);

    Matrix_TranslateRel32(bone[4].pos);
    Matrix_Rot16(mesh_rots[LM_CALF_R]);
    M_DrawMesh(LM_CALF_R, clip, false);

    Matrix_TranslateRel32(bone[5].pos);
    Matrix_Rot16(mesh_rots[LM_FOOT_R]);
    M_DrawMesh(LM_FOOT_R, clip, false);

    Matrix_Pop();

    Matrix_TranslateRel32(bone[6].pos);
    Matrix_Rot16(mesh_rots[LM_TORSO]);
    Matrix_Rot16(g_Lara.interp.result.torso_rot);
    M_DrawMesh(LM_TORSO, clip, false);

    Matrix_Push();

    Matrix_TranslateRel32(bone[13].pos);
    Matrix_Rot16(mesh_rots[LM_HEAD]);
    Matrix_Rot16(g_Lara.interp.result.head_rot);
    M_DrawMesh(LM_HEAD, clip, false);

    *g_MatrixPtr = saved_matrix;
    Lara_Hair_Draw();

    Matrix_Pop();

    int32_t fire_arms = 0;
    if (g_Lara.gun_status == LGS_READY || g_Lara.gun_status == LGS_DRAW
        || g_Lara.gun_status == LGS_UNDRAW) {
        fire_arms = g_Lara.gun_type;
    }

    switch (fire_arms) {
    case LGT_UNARMED:
        Matrix_Push();

        Matrix_TranslateRel32(bone[7].pos);
        Matrix_Rot16(mesh_rots[LM_UARM_R]);
        M_DrawMesh(LM_UARM_R, clip, false);

        Matrix_TranslateRel32(bone[8].pos);
        Matrix_Rot16(mesh_rots[LM_LARM_R]);
        M_DrawMesh(LM_LARM_R, clip, false);

        Matrix_TranslateRel32(bone[9].pos);
        Matrix_Rot16(mesh_rots[LM_HAND_R]);
        M_DrawMesh(LM_HAND_R, clip, false);

        Matrix_Pop();

        Matrix_Push();

        Matrix_TranslateRel32(bone[10].pos);
        Matrix_Rot16(mesh_rots[LM_UARM_L]);
        M_DrawMesh(LM_UARM_L, clip, false);

        Matrix_TranslateRel32(bone[11].pos);
        Matrix_Rot16(mesh_rots[LM_LARM_L]);
        M_DrawMesh(LM_LARM_L, clip, false);

        Matrix_TranslateRel32(bone[12].pos);
        Matrix_Rot16(mesh_rots[LM_HAND_L]);
        M_DrawMesh(LM_HAND_L, clip, false);

        Matrix_Pop();
        break;

    case LGT_PISTOLS:
    case LGT_MAGNUMS:
    case LGT_UZIS:
        Matrix_Push();

        Matrix_TranslateRel32(bone[7].pos);

        g_MatrixPtr->_00 = g_MatrixPtr[-2]._00;
        g_MatrixPtr->_01 = g_MatrixPtr[-2]._01;
        g_MatrixPtr->_02 = g_MatrixPtr[-2]._02;
        g_MatrixPtr->_10 = g_MatrixPtr[-2]._10;
        g_MatrixPtr->_11 = g_MatrixPtr[-2]._11;
        g_MatrixPtr->_12 = g_MatrixPtr[-2]._12;
        g_MatrixPtr->_20 = g_MatrixPtr[-2]._20;
        g_MatrixPtr->_21 = g_MatrixPtr[-2]._21;
        g_MatrixPtr->_22 = g_MatrixPtr[-2]._22;

        mesh_rots =
            g_Lara.right_arm.frame_base[g_Lara.right_arm.frame_num].mesh_rots;
        Matrix_Rot16(g_Lara.right_arm.interp.result.rot);
        Matrix_Rot16(mesh_rots[LM_UARM_R]);
        M_DrawMesh(LM_UARM_R, clip, false);

        Matrix_TranslateRel32(bone[8].pos);
        Matrix_Rot16(mesh_rots[LM_LARM_R]);
        M_DrawMesh(LM_LARM_R, clip, false);

        Matrix_TranslateRel32(bone[9].pos);
        Matrix_Rot16(mesh_rots[LM_HAND_R]);
        M_DrawMesh(LM_HAND_R, clip, false);

        if (g_Lara.right_arm.flash_gun) {
            saved_matrix = *g_MatrixPtr;
        }

        Matrix_Pop();

        Matrix_Push();

        Matrix_TranslateRel32(bone[10].pos);

        g_MatrixPtr->_00 = g_MatrixPtr[-2]._00;
        g_MatrixPtr->_01 = g_MatrixPtr[-2]._01;
        g_MatrixPtr->_02 = g_MatrixPtr[-2]._02;
        g_MatrixPtr->_10 = g_MatrixPtr[-2]._10;
        g_MatrixPtr->_11 = g_MatrixPtr[-2]._11;
        g_MatrixPtr->_12 = g_MatrixPtr[-2]._12;
        g_MatrixPtr->_20 = g_MatrixPtr[-2]._20;
        g_MatrixPtr->_21 = g_MatrixPtr[-2]._21;
        g_MatrixPtr->_22 = g_MatrixPtr[-2]._22;

        mesh_rots =
            g_Lara.left_arm.frame_base[g_Lara.left_arm.frame_num].mesh_rots;
        Matrix_Rot16(g_Lara.left_arm.interp.result.rot);
        Matrix_Rot16(mesh_rots[LM_UARM_L]);
        M_DrawMesh(LM_UARM_L, clip, false);

        Matrix_TranslateRel32(bone[11].pos);
        Matrix_Rot16(mesh_rots[LM_LARM_L]);
        M_DrawMesh(LM_LARM_L, clip, false);

        Matrix_TranslateRel32(bone[12].pos);
        Matrix_Rot16(mesh_rots[LM_HAND_L]);
        M_DrawMesh(LM_HAND_L, clip, false);

        if (g_Lara.left_arm.flash_gun) {
            Gun_DrawFlash(fire_arms, clip);
        }
        if (g_Lara.right_arm.flash_gun) {
            *g_MatrixPtr = saved_matrix;
            Gun_DrawFlash(fire_arms, clip);
        }

        Matrix_Pop();
        break;

    case LGT_SHOTGUN:
        Matrix_Push();

        mesh_rots =
            g_Lara.right_arm.frame_base[g_Lara.right_arm.frame_num].mesh_rots;
        Matrix_TranslateRel32(bone[7].pos);
        Matrix_Rot16(mesh_rots[LM_UARM_R]);
        M_DrawMesh(LM_UARM_R, clip, false);

        Matrix_TranslateRel32(bone[8].pos);
        Matrix_Rot16(mesh_rots[LM_LARM_R]);
        M_DrawMesh(LM_LARM_R, clip, false);

        Matrix_TranslateRel32(bone[9].pos);
        Matrix_Rot16(mesh_rots[LM_HAND_R]);
        M_DrawMesh(LM_HAND_R, clip, false);

        if (g_Lara.right_arm.flash_gun) {
            saved_matrix = *g_MatrixPtr;
        }

        Matrix_Pop();

        Matrix_Push();

        mesh_rots =
            g_Lara.left_arm.frame_base[g_Lara.left_arm.frame_num].mesh_rots;
        Matrix_TranslateRel32(bone[10].pos);
        Matrix_Rot16(mesh_rots[LM_UARM_L]);
        M_DrawMesh(LM_UARM_L, clip, false);

        Matrix_TranslateRel32(bone[11].pos);
        Matrix_Rot16(mesh_rots[LM_LARM_L]);
        M_DrawMesh(LM_LARM_L, clip, false);

        Matrix_TranslateRel32(bone[12].pos);
        Matrix_Rot16(mesh_rots[LM_HAND_L]);
        M_DrawMesh(LM_HAND_L, clip, false);

        if (g_Lara.right_arm.flash_gun) {
            *g_MatrixPtr = saved_matrix;
            Gun_DrawFlash(fire_arms, clip);
        }

        Matrix_Pop();
        break;
    }

    Matrix_Pop();
    Matrix_Pop();

end:
    g_PhdLeft = left;
    g_PhdRight = right;
    g_PhdTop = top;
    g_PhdBottom = bottom;
}

void Lara_Draw_I(
    const ITEM *const item, ANIM_FRAME *frame1, ANIM_FRAME *frame2,
    int32_t frac, int32_t rate)
{
    MATRIX saved_matrix;

    const OBJECT *const obj = Object_Get(item->object_id);
    const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(item);

    saved_matrix = *g_MatrixPtr;

    Output_DrawShadow(obj->shadow_size, bounds, item);

    Matrix_Push();
    Matrix_TranslateAbs32(item->interp.result.pos);
    Matrix_Rot16(item->interp.result.rot);

    int32_t clip = Output_GetObjectBounds(&frame1->bounds);
    if (!clip) {
        Matrix_Pop();
        return;
    }

    Matrix_Push();

    Output_CalculateObjectLighting(item, &frame1->bounds);

    const ANIM_BONE *const bone = Object_GetBone(obj, 0);
    const XYZ_16 *mesh_rots_1 = frame1->mesh_rots;
    const XYZ_16 *mesh_rots_2 = frame2->mesh_rots;

    Matrix_InitInterpolate(frac, rate);

    Matrix_TranslateRel16_ID(frame1->offset, frame2->offset);
    Matrix_Rot16_ID(mesh_rots_1[LM_HIPS], mesh_rots_2[LM_HIPS]);
    M_DrawMesh(LM_HIPS, clip, true);

    Matrix_Push_I();

    Matrix_TranslateRel32_I(bone[0].pos);
    Matrix_Rot16_ID(mesh_rots_1[LM_THIGH_L], mesh_rots_2[LM_THIGH_L]);
    M_DrawMesh(LM_THIGH_L, clip, true);

    Matrix_TranslateRel32_I(bone[1].pos);
    Matrix_Rot16_ID(mesh_rots_1[LM_CALF_L], mesh_rots_2[LM_CALF_L]);
    M_DrawMesh(LM_CALF_L, clip, true);

    Matrix_TranslateRel32_I(bone[2].pos);
    Matrix_Rot16_ID(mesh_rots_1[LM_FOOT_L], mesh_rots_2[LM_FOOT_L]);
    M_DrawMesh(LM_FOOT_L, clip, true);

    Matrix_Pop_I();

    Matrix_Push_I();

    Matrix_TranslateRel32_I(bone[3].pos);
    Matrix_Rot16_ID(mesh_rots_1[LM_THIGH_R], mesh_rots_2[LM_THIGH_R]);
    M_DrawMesh(LM_THIGH_R, clip, true);

    Matrix_TranslateRel32_I(bone[4].pos);
    Matrix_Rot16_ID(mesh_rots_1[LM_CALF_R], mesh_rots_2[LM_CALF_R]);
    M_DrawMesh(LM_CALF_R, clip, true);

    Matrix_TranslateRel32_I(bone[5].pos);
    Matrix_Rot16_ID(mesh_rots_1[LM_FOOT_R], mesh_rots_2[LM_FOOT_R]);
    M_DrawMesh(LM_FOOT_R, clip, true);

    Matrix_Pop_I();

    Matrix_TranslateRel32_I(bone[6].pos);
    Matrix_Rot16_ID(mesh_rots_1[LM_TORSO], mesh_rots_2[LM_TORSO]);
    Matrix_Rot16_I(g_Lara.interp.result.torso_rot);
    M_DrawMesh(LM_TORSO, clip, true);

    Matrix_Push_I();

    Matrix_TranslateRel32_I(bone[13].pos);
    Matrix_Rot16_ID(mesh_rots_1[LM_HEAD], mesh_rots_2[LM_HEAD]);
    Matrix_Rot16_I(g_Lara.interp.result.head_rot);
    M_DrawMesh(LM_HEAD, clip, true);

    *g_MatrixPtr = saved_matrix;
    Lara_Hair_Draw();

    Matrix_Pop_I();

    int32_t fire_arms = 0;
    if (g_Lara.gun_status == LGS_READY || g_Lara.gun_status == LGS_DRAW
        || g_Lara.gun_status == LGS_UNDRAW) {
        fire_arms = g_Lara.gun_type;
    }

    switch (fire_arms) {
    case LGT_UNARMED:
        Matrix_Push_I();

        Matrix_TranslateRel32_I(bone[7].pos);
        Matrix_Rot16_ID(mesh_rots_1[LM_UARM_R], mesh_rots_2[LM_UARM_R]);
        M_DrawMesh(LM_UARM_R, clip, true);

        Matrix_TranslateRel32_I(bone[8].pos);
        Matrix_Rot16_ID(mesh_rots_1[LM_LARM_R], mesh_rots_2[LM_LARM_R]);
        M_DrawMesh(LM_LARM_R, clip, true);

        Matrix_TranslateRel32_I(bone[9].pos);
        Matrix_Rot16_ID(mesh_rots_1[LM_HAND_R], mesh_rots_2[LM_HAND_R]);
        M_DrawMesh(LM_HAND_R, clip, true);

        Matrix_Pop_I();

        Matrix_Push_I();

        Matrix_TranslateRel32_I(bone[10].pos);
        Matrix_Rot16_ID(mesh_rots_1[LM_UARM_L], mesh_rots_2[LM_UARM_L]);
        M_DrawMesh(LM_UARM_L, clip, true);

        Matrix_TranslateRel32_I(bone[11].pos);
        Matrix_Rot16_ID(mesh_rots_1[LM_LARM_L], mesh_rots_2[LM_LARM_L]);
        M_DrawMesh(LM_LARM_L, clip, true);

        Matrix_TranslateRel32_I(bone[12].pos);
        Matrix_Rot16_ID(mesh_rots_1[LM_HAND_L], mesh_rots_2[LM_HAND_L]);
        M_DrawMesh(LM_HAND_L, clip, true);

        Matrix_Pop_I();
        break;

    case LGT_PISTOLS:
    case LGT_MAGNUMS:
    case LGT_UZIS:
        Matrix_Push_I();

        Matrix_TranslateRel32_I(bone[7].pos);
        Matrix_InterpolateArm();

        mesh_rots_1 =
            g_Lara.right_arm.frame_base[g_Lara.right_arm.frame_num].mesh_rots;
        Matrix_Rot16(g_Lara.right_arm.interp.result.rot);
        Matrix_Rot16(mesh_rots_1[LM_UARM_R]);
        M_DrawMesh(LM_UARM_R, clip, false);

        Matrix_TranslateRel32(bone[8].pos);
        Matrix_Rot16(mesh_rots_1[LM_LARM_R]);
        M_DrawMesh(LM_LARM_R, clip, false);

        Matrix_TranslateRel32(bone[9].pos);
        Matrix_Rot16(mesh_rots_1[LM_HAND_R]);
        M_DrawMesh(LM_HAND_R, clip, false);

        if (g_Lara.right_arm.flash_gun) {
            saved_matrix = *g_MatrixPtr;
        }

        Matrix_Pop_I();

        Matrix_Push_I();

        Matrix_TranslateRel32_I(bone[10].pos);
        Matrix_InterpolateArm();

        mesh_rots_1 =
            g_Lara.left_arm.frame_base[g_Lara.left_arm.frame_num].mesh_rots;
        Matrix_Rot16(g_Lara.left_arm.interp.result.rot);
        Matrix_Rot16(mesh_rots_1[LM_UARM_L]);
        M_DrawMesh(LM_UARM_L, clip, false);

        Matrix_TranslateRel32(bone[11].pos);
        Matrix_Rot16(mesh_rots_1[LM_LARM_L]);
        M_DrawMesh(LM_LARM_L, clip, false);

        Matrix_TranslateRel32(bone[12].pos);
        Matrix_Rot16(mesh_rots_1[LM_HAND_L]);
        M_DrawMesh(LM_HAND_L, clip, false);

        if (g_Lara.left_arm.flash_gun) {
            Gun_DrawFlash(fire_arms, clip);
        }

        if (g_Lara.right_arm.flash_gun) {
            *g_MatrixPtr = saved_matrix;
            Gun_DrawFlash(fire_arms, clip);
        }

        Matrix_Pop_I();
        break;

    case LGT_SHOTGUN:
        Matrix_Push_I();

        mesh_rots_1 =
            g_Lara.right_arm.frame_base[g_Lara.right_arm.frame_num].mesh_rots;
        mesh_rots_2 = mesh_rots_1;
        Matrix_TranslateRel32_I(bone[7].pos);
        Matrix_Rot16_ID(mesh_rots_1[LM_UARM_R], mesh_rots_2[LM_UARM_R]);
        M_DrawMesh(LM_UARM_R, clip, true);

        Matrix_TranslateRel32_I(bone[8].pos);
        Matrix_Rot16_ID(mesh_rots_1[LM_LARM_R], mesh_rots_2[LM_LARM_R]);
        M_DrawMesh(LM_LARM_R, clip, true);

        Matrix_TranslateRel32_I(bone[9].pos);
        Matrix_Rot16_ID(mesh_rots_1[LM_HAND_R], mesh_rots_2[LM_HAND_R]);
        M_DrawMesh(LM_HAND_R, clip, true);

        if (g_Lara.right_arm.flash_gun) {
            saved_matrix = *g_MatrixPtr;
        }

        Matrix_Pop_I();

        Matrix_Push_I();

        mesh_rots_1 =
            g_Lara.left_arm.frame_base[g_Lara.left_arm.frame_num].mesh_rots;
        mesh_rots_2 = mesh_rots_1;
        Matrix_TranslateRel32_I(bone[10].pos);
        Matrix_Rot16_ID(mesh_rots_1[LM_UARM_L], mesh_rots_2[LM_UARM_L]);
        M_DrawMesh(LM_UARM_L, clip, true);

        Matrix_TranslateRel32_I(bone[11].pos);
        Matrix_Rot16_ID(mesh_rots_1[LM_LARM_L], mesh_rots_2[LM_LARM_L]);
        M_DrawMesh(LM_LARM_L, clip, true);

        Matrix_TranslateRel32_I(bone[12].pos);
        Matrix_Rot16_ID(mesh_rots_1[LM_HAND_L], mesh_rots_2[LM_HAND_L]);
        M_DrawMesh(LM_HAND_L, clip, true);

        if (g_Lara.right_arm.flash_gun) {
            *g_MatrixPtr = saved_matrix;
            Gun_DrawFlash(fire_arms, clip);
        }

        Matrix_Pop_I();
        break;
    }

    Matrix_Pop();
    Matrix_Pop();
}
