#include "game/lara/draw.h"

#include "game/gun/gun_misc.h"
#include "game/items.h"
#include "game/lara/hair.h"
#include "game/lara/misc.h"
#include "game/output.h"
#include "game/random.h"
#include "global/vars.h"

#include <libtrx/game/lara/common.h>
#include <libtrx/game/matrix.h>

static void M_DrawBodyPart(
    LARA_MESH mesh, const ANIM_BONE *bone, const XYZ_16 *mesh_rots_1,
    const XYZ_16 *mesh_rots_2, int32_t clip);

static void M_DrawBodyPart(
    const LARA_MESH mesh, const ANIM_BONE *const bone,
    const XYZ_16 *mesh_rots_1, const XYZ_16 *mesh_rots_2, const int32_t clip)
{
    if (mesh_rots_2 != nullptr) {
        Matrix_TranslateRel32_I(bone[mesh - 1].pos);
        Matrix_Rot16_ID(mesh_rots_1[mesh], mesh_rots_2[mesh]);
        Output_DrawObjectMesh_I(g_Lara.mesh_ptrs[mesh], clip);
    } else {
        Matrix_TranslateRel32(bone[mesh - 1].pos);
        Matrix_Rot16(mesh_rots_1[mesh]);
        Output_DrawObjectMesh(g_Lara.mesh_ptrs[mesh], clip);
    }
}

void Lara_Draw(const ITEM *const item)
{
    MATRIX saved_matrix;

    const int32_t top = g_PhdWinTop;
    const int32_t left = g_PhdWinLeft;
    const int32_t right = g_PhdWinRight;
    const int32_t bottom = g_PhdWinBottom;

    g_PhdWinTop = 0;
    g_PhdWinLeft = 0;
    g_PhdWinBottom = g_PhdWinMaxY;
    g_PhdWinRight = g_PhdWinMaxX;

    ANIM_FRAME *frames[2];
    if (g_Lara.hit_direction < 0) {
        int32_t rate;
        const int32_t frac = Item_GetFrames(item, frames, &rate);
        if (frac) {
            Lara_Draw_I(item, frames[0], frames[1], frac, rate);
            goto finish;
        }
    }

    const ANIM_FRAME *const hit_frame = Lara_GetHitFrame(item);
    const ANIM_FRAME *const frame =
        hit_frame == nullptr ? frames[0] : hit_frame;

    const OBJECT *const obj = Object_Get(item->object_id);
    if (g_Lara.skidoo == NO_ITEM) {
        Output_InsertShadow(obj->shadow_size, &frame->bounds, item);
    }

    saved_matrix = *g_MatrixPtr;

    Matrix_Push();
    Matrix_TranslateAbs32(item->pos);
    Matrix_Rot16(item->rot);
    const MATRIX item_matrix = *g_MatrixPtr;
    const int32_t clip = Output_GetObjectBounds(&frame->bounds);
    if (!clip) {
        Matrix_Pop();
        return;
    }

    Matrix_Push();
    Output_CalculateObjectLighting(item, &frame->bounds);

    const ANIM_BONE *const bone = Object_GetBone(obj, 0);
    const XYZ_16 *mesh_rots = frame->mesh_rots;
    const XYZ_16 *mesh_rots_c;

    Matrix_TranslateRel16(frame->offset);
    Matrix_Rot16(mesh_rots[LM_HIPS]);
    Output_DrawObjectMesh(g_Lara.mesh_ptrs[LM_HIPS], clip);

    Matrix_Push();
    M_DrawBodyPart(LM_THIGH_L, bone, mesh_rots, nullptr, clip);
    M_DrawBodyPart(LM_CALF_L, bone, mesh_rots, nullptr, clip);
    M_DrawBodyPart(LM_FOOT_L, bone, mesh_rots, nullptr, clip);
    Matrix_Pop();

    Matrix_Push();
    M_DrawBodyPart(LM_THIGH_R, bone, mesh_rots, nullptr, clip);
    M_DrawBodyPart(LM_CALF_R, bone, mesh_rots, nullptr, clip);
    M_DrawBodyPart(LM_FOOT_R, bone, mesh_rots, nullptr, clip);
    Matrix_Pop();

    Matrix_TranslateRel32(bone[6].pos);
    if (Lara_IsM16Active()) {
        mesh_rots =
            g_Lara.right_arm.frame_base[g_Lara.right_arm.frame_num].mesh_rots;
    }

    Matrix_Rot16(mesh_rots[LM_TORSO]);
    Matrix_Rot16(g_Lara.torso_rot);
    Output_DrawObjectMesh(g_Lara.mesh_ptrs[LM_TORSO], clip);

    Matrix_Push();
    Matrix_TranslateRel32(bone[13].pos);
    mesh_rots_c = mesh_rots;
    Matrix_Rot16(mesh_rots[LM_HEAD]);
    mesh_rots = mesh_rots_c;
    Matrix_Rot16(g_Lara.head_rot);
    Output_DrawObjectMesh(g_Lara.mesh_ptrs[LM_HEAD], clip);

    *g_MatrixPtr = saved_matrix;
    Lara_Hair_Draw();

    Matrix_Pop();

    if (g_Lara.back_gun) {
        Matrix_Push();
        const OBJECT *const back_obj = Object_Get(g_Lara.back_gun);
        const ANIM_BONE *const bone_c = Object_GetBone(back_obj, 0);
        Matrix_TranslateRel32(bone_c[13].pos);
        mesh_rots_c = back_obj->frame_base->mesh_rots;
        Matrix_Rot16(mesh_rots_c[LM_HEAD]);
        Object_DrawMesh(back_obj->mesh_idx + LM_HEAD, clip, false);
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
        M_DrawBodyPart(LM_UARM_R, bone, mesh_rots, nullptr, clip);
        M_DrawBodyPart(LM_LARM_R, bone, mesh_rots, nullptr, clip);
        M_DrawBodyPart(LM_HAND_R, bone, mesh_rots, nullptr, clip);
        Matrix_Pop();

        Matrix_Push();
        Matrix_TranslateRel32(bone[10].pos);
        if (g_Lara.flare_control_left) {
            const ANIM *const anim = Anim_GetAnim(g_Lara.left_arm.anim_num);
            mesh_rots =
                g_Lara.left_arm
                    .frame_base[g_Lara.left_arm.frame_num - anim->frame_base]
                    .mesh_rots;
        }

        Matrix_Rot16(mesh_rots[LM_UARM_L]);
        Output_DrawObjectMesh(g_Lara.mesh_ptrs[LM_UARM_L], clip);

        M_DrawBodyPart(LM_LARM_L, bone, mesh_rots, nullptr, clip);
        M_DrawBodyPart(LM_HAND_L, bone, mesh_rots, nullptr, clip);

        if (g_Lara.gun_type == LGT_FLARE && g_Lara.left_arm.flash_gun) {
            Gun_DrawFlash(LGT_FLARE, clip);
        }

        Matrix_Pop();
        break;

    case LGT_PISTOLS:
    case LGT_MAGNUMS:
    case LGT_UZIS: {
        Matrix_Push();
        Matrix_TranslateRel32(bone[7].pos);
        g_MatrixPtr->_00 = item_matrix._00;
        g_MatrixPtr->_01 = item_matrix._01;
        g_MatrixPtr->_02 = item_matrix._02;
        g_MatrixPtr->_10 = item_matrix._10;
        g_MatrixPtr->_11 = item_matrix._11;
        g_MatrixPtr->_12 = item_matrix._12;
        g_MatrixPtr->_20 = item_matrix._20;
        g_MatrixPtr->_21 = item_matrix._21;
        g_MatrixPtr->_22 = item_matrix._22;
        Matrix_Rot16(g_Lara.right_arm.rot);
        const ANIM *anim = Anim_GetAnim(g_Lara.right_arm.anim_num);
        mesh_rots =
            g_Lara.right_arm
                .frame_base[g_Lara.right_arm.frame_num - anim->frame_base]
                .mesh_rots;
        Matrix_Rot16(mesh_rots[LM_UARM_R]);
        Output_DrawObjectMesh(g_Lara.mesh_ptrs[LM_UARM_R], clip);

        M_DrawBodyPart(LM_LARM_R, bone, mesh_rots, nullptr, clip);
        M_DrawBodyPart(LM_HAND_R, bone, mesh_rots, nullptr, clip);

        if (g_Lara.right_arm.flash_gun) {
            saved_matrix = *g_MatrixPtr;
        }
        Matrix_Pop();

        Matrix_Push();
        Matrix_TranslateRel32(bone[10].pos);
        g_MatrixPtr->_00 = item_matrix._00;
        g_MatrixPtr->_01 = item_matrix._01;
        g_MatrixPtr->_02 = item_matrix._02;
        g_MatrixPtr->_10 = item_matrix._10;
        g_MatrixPtr->_11 = item_matrix._11;
        g_MatrixPtr->_12 = item_matrix._12;
        g_MatrixPtr->_20 = item_matrix._20;
        g_MatrixPtr->_21 = item_matrix._21;
        g_MatrixPtr->_22 = item_matrix._22;
        Matrix_Rot16(g_Lara.left_arm.rot);
        anim = Anim_GetAnim(g_Lara.left_arm.anim_num);
        mesh_rots =
            g_Lara.left_arm
                .frame_base[g_Lara.left_arm.frame_num - anim->frame_base]
                .mesh_rots;
        Matrix_Rot16(mesh_rots[LM_UARM_L]);
        Output_DrawObjectMesh(g_Lara.mesh_ptrs[LM_UARM_L], clip);

        M_DrawBodyPart(LM_LARM_L, bone, mesh_rots, nullptr, clip);
        M_DrawBodyPart(LM_HAND_L, bone, mesh_rots, nullptr, clip);

        if (g_Lara.left_arm.flash_gun) {
            Gun_DrawFlash(gun_type, clip);
        }
        if (g_Lara.right_arm.flash_gun) {
            *g_MatrixPtr = saved_matrix;
            Gun_DrawFlash(gun_type, clip);
        }

        Matrix_Pop();
        break;
    }

    case LGT_SHOTGUN:
    case LGT_M16:
    case LGT_GRENADE:
    case LGT_HARPOON: {
        Matrix_Push();
        Matrix_TranslateRel32(bone[7].pos);
        mesh_rots =
            g_Lara.right_arm.frame_base[g_Lara.right_arm.frame_num].mesh_rots;
        Matrix_Rot16(mesh_rots[LM_UARM_R]);
        Output_DrawObjectMesh(g_Lara.mesh_ptrs[LM_UARM_R], clip);

        M_DrawBodyPart(LM_LARM_R, bone, mesh_rots, nullptr, clip);
        M_DrawBodyPart(LM_HAND_R, bone, mesh_rots, nullptr, clip);

        if (g_Lara.right_arm.flash_gun) {
            saved_matrix = *g_MatrixPtr;
        }
        Matrix_Pop();

        Matrix_Push();
        M_DrawBodyPart(LM_UARM_L, bone, mesh_rots, nullptr, clip);
        M_DrawBodyPart(LM_LARM_L, bone, mesh_rots, nullptr, clip);
        M_DrawBodyPart(LM_HAND_L, bone, mesh_rots, nullptr, clip);

        if (g_Lara.right_arm.flash_gun) {
            *g_MatrixPtr = saved_matrix;
            Gun_DrawFlash(gun_type, clip);
        }

        Matrix_Pop();
        break;
    }

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

void Lara_Draw_I(
    const ITEM *const item, const ANIM_FRAME *const frame1,
    const ANIM_FRAME *const frame2, const int32_t frac, const int32_t rate)
{
    const OBJECT *const obj = Object_Get(item->object_id);
    const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(item);

    if (g_Lara.skidoo == NO_ITEM) {
        Output_InsertShadow(obj->shadow_size, bounds, item);
    }

    MATRIX saved_matrix = *g_MatrixPtr;

    Matrix_Push();
    Matrix_TranslateAbs32(item->pos);
    Matrix_Rot16(item->rot);

    const int32_t clip = Output_GetObjectBounds(&frame1->bounds);

    if (!clip) {
        Matrix_Pop();
        return;
    }

    Matrix_Push();
    Output_CalculateObjectLighting(item, &frame1->bounds);

    const ANIM_BONE *const bone = Object_GetBone(obj, 0);
    const XYZ_16 *mesh_rots_1 = frame1->mesh_rots;
    const XYZ_16 *mesh_rots_2 = frame2->mesh_rots;
    const XYZ_16 *mesh_rots_1_c;
    const XYZ_16 *mesh_rots_2_c;

    Matrix_InitInterpolate(frac, rate);
    Matrix_TranslateRel16_ID(frame1->offset, frame2->offset);
    Matrix_Rot16_ID(mesh_rots_1[LM_HIPS], mesh_rots_2[LM_HIPS]);
    Output_DrawObjectMesh_I(g_Lara.mesh_ptrs[LM_HIPS], clip);

    Matrix_Push_I();
    M_DrawBodyPart(LM_THIGH_L, bone, mesh_rots_1, mesh_rots_2, clip);
    M_DrawBodyPart(LM_CALF_L, bone, mesh_rots_1, mesh_rots_2, clip);
    M_DrawBodyPart(LM_FOOT_L, bone, mesh_rots_1, mesh_rots_2, clip);
    Matrix_Pop_I();

    Matrix_Push_I();
    M_DrawBodyPart(LM_THIGH_R, bone, mesh_rots_1, mesh_rots_2, clip);
    M_DrawBodyPart(LM_CALF_R, bone, mesh_rots_1, mesh_rots_2, clip);
    M_DrawBodyPart(LM_FOOT_R, bone, mesh_rots_1, mesh_rots_2, clip);
    Matrix_Pop_I();

    Matrix_TranslateRel32_I(bone[6].pos);
    if (Lara_IsM16Active()) {
        mesh_rots_2 =
            g_Lara.right_arm.frame_base[g_Lara.right_arm.frame_num].mesh_rots;
        mesh_rots_1 = mesh_rots_2;
    }

    Matrix_Rot16_ID(mesh_rots_1[LM_TORSO], mesh_rots_2[LM_TORSO]);
    Matrix_Rot16_I(g_Lara.torso_rot);
    Output_DrawObjectMesh_I(g_Lara.mesh_ptrs[LM_TORSO], clip);

    Matrix_Push_I();
    Matrix_TranslateRel32_I(bone[13].pos);
    mesh_rots_1_c = mesh_rots_1;
    mesh_rots_2_c = mesh_rots_2;
    Matrix_Rot16_ID(mesh_rots_1[LM_HEAD], mesh_rots_2[LM_HEAD]);
    mesh_rots_1 = mesh_rots_1_c;
    mesh_rots_2 = mesh_rots_2_c;
    Matrix_Rot16_I(g_Lara.head_rot);
    Output_DrawObjectMesh_I(g_Lara.mesh_ptrs[LM_HEAD], clip);

    *g_MatrixPtr = saved_matrix;
    Lara_Hair_Draw();
    Matrix_Pop_I();

    if (g_Lara.back_gun) {
        Matrix_Push_I();
        const OBJECT *const back_obj = Object_Get(g_Lara.back_gun);
        const ANIM_BONE *const bone_c = Object_GetBone(back_obj, 0);
        Matrix_TranslateRel32_I(bone_c[13].pos);
        mesh_rots_1_c = back_obj->frame_base->mesh_rots;
        mesh_rots_2_c = back_obj->frame_base->mesh_rots;
        Matrix_Rot16_ID(mesh_rots_1_c[LM_HEAD], mesh_rots_2_c[LM_HEAD]);
        Object_DrawMesh(back_obj->mesh_idx + LM_HEAD, clip, true);
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
        M_DrawBodyPart(LM_UARM_R, bone, mesh_rots_1, mesh_rots_2, clip);
        M_DrawBodyPart(LM_LARM_R, bone, mesh_rots_1, mesh_rots_2, clip);
        M_DrawBodyPart(LM_HAND_R, bone, mesh_rots_1, mesh_rots_2, clip);
        Matrix_Pop_I();

        Matrix_Push_I();
        Matrix_TranslateRel32_I(bone[10].pos);
        if (g_Lara.flare_control_left) {
            const ANIM *const anim = Anim_GetAnim(g_Lara.left_arm.anim_num);
            mesh_rots_1 =
                g_Lara.left_arm
                    .frame_base[g_Lara.left_arm.frame_num - anim->frame_base]
                    .mesh_rots;
            mesh_rots_2 = mesh_rots_1;
        }

        Matrix_Rot16_ID(mesh_rots_1[LM_UARM_L], mesh_rots_2[LM_UARM_L]);
        Output_DrawObjectMesh_I(g_Lara.mesh_ptrs[LM_UARM_L], clip);

        M_DrawBodyPart(LM_LARM_L, bone, mesh_rots_1, mesh_rots_2, clip);
        M_DrawBodyPart(LM_HAND_L, bone, mesh_rots_1, mesh_rots_2, clip);

        if (g_Lara.gun_type == LGT_FLARE && g_Lara.left_arm.flash_gun) {
            Matrix_TranslateRel_I(11, 32, 80);
            Matrix_RotX_I(-90 * DEG_1);
            Matrix_RotY_I(2 * Random_GetDraw());
            Output_CalculateStaticLight(2048);
            Object_DrawMesh(Object_Get(O_FLARE_FIRE)->mesh_idx, clip, true);
        }
        Matrix_Pop();
        break;

    case LGT_PISTOLS:
    case LGT_MAGNUMS:
    case LGT_UZIS: {
        Matrix_Push_I();
        Matrix_TranslateRel32_I(bone[7].pos);
        Matrix_InterpolateArm();
        Matrix_Rot16(g_Lara.right_arm.rot);
        const ANIM *anim = Anim_GetAnim(g_Lara.right_arm.anim_num);
        mesh_rots_1 =
            g_Lara.right_arm
                .frame_base[g_Lara.right_arm.frame_num - anim->frame_base]
                .mesh_rots;
        Matrix_Rot16(mesh_rots_1[LM_UARM_R]);
        Output_DrawObjectMesh(g_Lara.mesh_ptrs[LM_UARM_R], clip);

        M_DrawBodyPart(LM_LARM_R, bone, mesh_rots_1, nullptr, clip);
        M_DrawBodyPart(LM_HAND_R, bone, mesh_rots_1, nullptr, clip);

        if (g_Lara.right_arm.flash_gun) {
            saved_matrix = *g_MatrixPtr;
        }
        Matrix_Pop_I();

        Matrix_Push_I();
        Matrix_TranslateRel32_I(bone[10].pos);
        Matrix_InterpolateArm();
        Matrix_Rot16(g_Lara.left_arm.rot);
        anim = Anim_GetAnim(g_Lara.left_arm.anim_num);
        mesh_rots_1 =
            g_Lara.left_arm
                .frame_base[g_Lara.left_arm.frame_num - anim->frame_base]
                .mesh_rots;
        Matrix_Rot16(mesh_rots_1[LM_UARM_L]);
        Output_DrawObjectMesh(g_Lara.mesh_ptrs[LM_UARM_L], clip);

        M_DrawBodyPart(LM_LARM_L, bone, mesh_rots_1, nullptr, clip);
        M_DrawBodyPart(LM_HAND_L, bone, mesh_rots_1, nullptr, clip);

        if (g_Lara.left_arm.flash_gun) {
            Gun_DrawFlash((int32_t)gun_type, clip);
        }
        if (g_Lara.right_arm.flash_gun) {
            *g_MatrixPtr = saved_matrix;
            Gun_DrawFlash(gun_type, clip);
        }
        Matrix_Pop();
        break;
    }

    case LGT_SHOTGUN:
    case LGT_M16:
    case LGT_GRENADE:
    case LGT_HARPOON: {
        Matrix_Push_I();
        Matrix_TranslateRel32_I(bone[7].pos);
        mesh_rots_1 =
            g_Lara.right_arm.frame_base[g_Lara.right_arm.frame_num].mesh_rots;
        mesh_rots_2 = mesh_rots_1;
        Matrix_Rot16_ID(mesh_rots_1[LM_UARM_R], mesh_rots_2[LM_UARM_R]);
        Output_DrawObjectMesh_I(g_Lara.mesh_ptrs[LM_UARM_R], clip);

        M_DrawBodyPart(LM_LARM_R, bone, mesh_rots_1, mesh_rots_2, clip);
        M_DrawBodyPart(LM_HAND_R, bone, mesh_rots_1, mesh_rots_2, clip);

        if (g_Lara.right_arm.flash_gun) {
            saved_matrix = *g_MatrixPtr;
        }
        Matrix_Pop_I();

        Matrix_Push_I();
        M_DrawBodyPart(LM_UARM_L, bone, mesh_rots_1, mesh_rots_2, clip);
        M_DrawBodyPart(LM_LARM_L, bone, mesh_rots_1, mesh_rots_2, clip);
        M_DrawBodyPart(LM_HAND_L, bone, mesh_rots_1, mesh_rots_2, clip);

        if (g_Lara.right_arm.flash_gun) {
            *g_MatrixPtr = saved_matrix;
            Gun_DrawFlash(gun_type, clip);
        }
        Matrix_Pop();
        break;
    }

    default:
        break;
    }

    Matrix_Pop();
    Matrix_Pop();
}
