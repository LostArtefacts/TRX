#include "game/lara/draw.h"

#include "game/gun/misc.h"
#include "game/lara.h"
#include "game/lara/pose.h"
#include "game/matrix.h"
#include "game/output.h"
#include "game/output/vars.h"
#include "game/random.h"

static void M_DrawBodyPart(
    const LARA_MESH mesh, const ANIM_BONE *const bone,
    const XYZ_16 *mesh_rots_1, const XYZ_16 *mesh_rots_2, const CLIP clip)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (mesh_rots_2 != nullptr) {
        Matrix_TranslateRel32_I(bone[mesh - 1].pos);
        Matrix_Rot16_ID(mesh_rots_1[mesh], mesh_rots_2[mesh]);
        Output_DrawObjectMesh_I(lara->mesh_ptrs[mesh], clip);
    } else {
        Matrix_TranslateRel32(bone[mesh - 1].pos);
        Matrix_Rot16(mesh_rots_1[mesh]);
        Output_DrawObjectMesh(lara->mesh_ptrs[mesh], clip);
    }
}

static void M_Draw_I(
    const ITEM *const item, const ANIM_FRAME *const frame1,
    const ANIM_FRAME *const frame2, const int32_t frac, const int32_t rate)
{
    const OBJECT *const obj = Object_Get(item->object_id);
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(item);

    if (!Lara_Vehicle_IsMounted()) {
        Output_DrawShadow(obj->shadow_size, bounds, item);
    }

    MATRIX saved_matrix = *g_MatrixPtr;

    Matrix_Push();
    Matrix_TranslateAbs32(item->interp.result.pos);
    Matrix_Rot16(item->interp.result.rot);

    const CLIP clip = Output_CheckBoundsClip(&frame1->bounds);
    if (clip == CLIP_NOT_VISIBLE) {
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
    Output_DrawObjectMesh_I(lara->mesh_ptrs[LM_HIPS], clip);

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

    Matrix_TranslateRel32_I(bone[LM_TORSO - 1].pos);
    if (Lara_IsM16Active()) {
        mesh_rots_2 =
            lara->right_arm.frame_base[lara->right_arm.frame_num].mesh_rots;
        mesh_rots_1 = mesh_rots_2;
    }

    Matrix_Rot16_ID(mesh_rots_1[LM_TORSO], mesh_rots_2[LM_TORSO]);
    Matrix_Rot16_I(lara->interp.result.torso_rot);
    Output_DrawObjectMesh_I(lara->mesh_ptrs[LM_TORSO], clip);

    Matrix_Push_I();
    Matrix_TranslateRel32_I(bone[LM_HEAD - 1].pos);
    mesh_rots_1_c = mesh_rots_1;
    mesh_rots_2_c = mesh_rots_2;
    Matrix_Rot16_ID(mesh_rots_1[LM_HEAD], mesh_rots_2[LM_HEAD]);
    mesh_rots_1 = mesh_rots_1_c;
    mesh_rots_2 = mesh_rots_2_c;
    Matrix_Rot16_I(lara->interp.result.head_rot);
    Output_DrawObjectMesh_I(lara->mesh_ptrs[LM_HEAD], clip);

    *g_MatrixPtr = saved_matrix;
    Lara_Hair_Draw();
    Matrix_Pop_I();

#if TR_VERSION >= 2
    if (lara->back_gun_obj_id != O_LARA) {
        Matrix_Push_I();
        const OBJECT *const back_obj = Object_Get(lara->back_gun_obj_id);
        const ANIM_BONE *const bone_c = Object_GetBone(back_obj, 0);
        Matrix_TranslateRel32_I(bone_c[13].pos);
        mesh_rots_1_c = back_obj->frame_base->mesh_rots;
        mesh_rots_2_c = back_obj->frame_base->mesh_rots;
        Matrix_Rot16_ID(mesh_rots_1_c[LM_HEAD], mesh_rots_2_c[LM_HEAD]);
        Object_DrawMesh(back_obj->mesh_idx + LM_HEAD, clip, true);
        Matrix_Pop_I();
    }
#endif

    LARA_GUN_TYPE gun_type = LGT_UNARMED;
    if (lara->gun_status == LGS_READY || lara->gun_status == LGS_SPECIAL
        || lara->gun_status == LGS_DRAW || lara->gun_status == LGS_UNDRAW) {
        gun_type = lara->gun_type;
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
        Matrix_TranslateRel32_I(bone[LM_UARM_L - 1].pos);
#if TR_VERSION >= 2
        if (lara->flare.control) {
            const ANIM *const anim = Anim_GetAnim(lara->left_arm.anim_num);
            mesh_rots_1 =
                lara->left_arm
                    .frame_base[lara->left_arm.frame_num - anim->frame_base]
                    .mesh_rots;
            mesh_rots_2 = mesh_rots_1;
        }
#endif

        Matrix_Rot16_ID(mesh_rots_1[LM_UARM_L], mesh_rots_2[LM_UARM_L]);
        Output_DrawObjectMesh_I(lara->mesh_ptrs[LM_UARM_L], clip);

        M_DrawBodyPart(LM_LARM_L, bone, mesh_rots_1, mesh_rots_2, clip);
        M_DrawBodyPart(LM_HAND_L, bone, mesh_rots_1, mesh_rots_2, clip);

        if (lara->gun_type == LGT_FLARE && lara->left_arm.flash_gun) {
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
        Matrix_TranslateRel32_I(bone[LM_UARM_R - 1].pos);
        Matrix_InterpolateArm();
        Matrix_Rot16(lara->right_arm.interp.result.rot);
#if TR_VERSION == 1
        mesh_rots_1 =
            lara->right_arm.frame_base[lara->right_arm.frame_num].mesh_rots;
#else
        const ANIM *anim = Anim_GetAnim(lara->right_arm.anim_num);
        mesh_rots_1 =
            lara->right_arm
                .frame_base[lara->right_arm.frame_num - anim->frame_base]
                .mesh_rots;
#endif
        Matrix_Rot16(mesh_rots_1[LM_UARM_R]);
        Output_DrawObjectMesh(lara->mesh_ptrs[LM_UARM_R], clip);

        M_DrawBodyPart(LM_LARM_R, bone, mesh_rots_1, nullptr, clip);
        M_DrawBodyPart(LM_HAND_R, bone, mesh_rots_1, nullptr, clip);

        if (lara->right_arm.flash_gun) {
            saved_matrix = *g_MatrixPtr;
        }
        Matrix_Pop_I();

        Matrix_Push_I();
        Matrix_TranslateRel32_I(bone[LM_UARM_L - 1].pos);
        Matrix_InterpolateArm();
        Matrix_Rot16(lara->left_arm.interp.result.rot);
#if TR_VERSION == 1
        mesh_rots_1 =
            lara->left_arm.frame_base[lara->left_arm.frame_num].mesh_rots;
#else
        anim = Anim_GetAnim(lara->left_arm.anim_num);
        mesh_rots_1 =
            lara->left_arm
                .frame_base[lara->left_arm.frame_num - anim->frame_base]
                .mesh_rots;
#endif
        Matrix_Rot16(mesh_rots_1[LM_UARM_L]);
        Output_DrawObjectMesh(lara->mesh_ptrs[LM_UARM_L], clip);

        M_DrawBodyPart(LM_LARM_L, bone, mesh_rots_1, nullptr, clip);
        M_DrawBodyPart(LM_HAND_L, bone, mesh_rots_1, nullptr, clip);

        if (lara->left_arm.flash_gun) {
            Gun_DrawFlash((int32_t)gun_type, clip);
        }
        if (lara->right_arm.flash_gun) {
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
        Matrix_TranslateRel32_I(bone[LM_UARM_R - 1].pos);
        mesh_rots_1 =
            lara->right_arm.frame_base[lara->right_arm.frame_num].mesh_rots;
        mesh_rots_2 = mesh_rots_1;
        Matrix_Rot16_ID(mesh_rots_1[LM_UARM_R], mesh_rots_2[LM_UARM_R]);
        Output_DrawObjectMesh_I(lara->mesh_ptrs[LM_UARM_R], clip);

// NOTE: gcc wrongly complains about mesh_rots_1 possibly being nullptr.
// While this is not the case, it's curious how the pistols subtract the
// frame_base from lara->*_arm.frame_num to access the mesh_rots, and the
// rifles do not.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"

        M_DrawBodyPart(LM_LARM_R, bone, mesh_rots_1, mesh_rots_2, clip);
        M_DrawBodyPart(LM_HAND_R, bone, mesh_rots_1, mesh_rots_2, clip);

        if (lara->right_arm.flash_gun) {
            saved_matrix = *g_MatrixPtr;
        }
        Matrix_Pop_I();

        Matrix_Push_I();
        M_DrawBodyPart(LM_UARM_L, bone, mesh_rots_1, mesh_rots_2, clip);
        M_DrawBodyPart(LM_LARM_L, bone, mesh_rots_1, mesh_rots_2, clip);
        M_DrawBodyPart(LM_HAND_L, bone, mesh_rots_1, mesh_rots_2, clip);

#pragma GCC diagnostic pop

        if (lara->right_arm.flash_gun) {
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

void Lara_Draw(const ITEM *const item)
{
    const int32_t top = g_PhdTop;
    const int32_t left = g_PhdLeft;
    const int32_t right = g_PhdRight;
    const int32_t bottom = g_PhdBottom;

    g_PhdLeft = Viewport_GetMinX(VIEWPORT_GAME);
    g_PhdRight = Viewport_GetMaxX(VIEWPORT_GAME);
    g_PhdTop = Viewport_GetMinY(VIEWPORT_GAME);
    g_PhdBottom = Viewport_GetMaxY(VIEWPORT_GAME);

    const LARA_INFO *const lara = Lara_GetLaraInfo();
    ANIM_FRAME *frames[2];
    if (lara->hit_direction < 0) {
        int32_t rate;
        const int32_t frac = Item_GetFrames(item, frames, &rate);
        if (frac != 0 && Lara_Pose_Get() == nullptr) {
            M_Draw_I(item, frames[0], frames[1], frac, rate);
            goto finish;
        }
    }

    const ANIM_FRAME *const hit_frame = Lara_GetHitFrame(item);
    const ANIM_FRAME *const frame =
        hit_frame == nullptr ? frames[0] : hit_frame;

    const OBJECT *const obj = Object_Get(item->object_id);
    if (!Lara_Vehicle_IsMounted()) {
        Output_DrawShadow(obj->shadow_size, &frame->bounds, item);
    }

    MATRIX saved_matrix = *g_MatrixPtr;

    Matrix_Push();
    Matrix_TranslateAbs32(item->interp.result.pos);
    Matrix_Rot16(item->interp.result.rot);
    const MATRIX item_matrix = *g_MatrixPtr;
    const CLIP clip = Output_CheckBoundsClip(&frame->bounds);
    if (clip == CLIP_NOT_VISIBLE) {
        Matrix_Pop();
        return;
    }

    Matrix_Push();
    Output_CalculateObjectLighting(item, &frame->bounds);

    const ANIM_BONE *const bone = Object_GetBone(obj, 0);
    const LARA_POSE *const pose = Lara_Pose_Get();
    const XYZ_16 *mesh_rots = pose != nullptr ? pose->rots : frame->mesh_rots;
    const XYZ_16 *mesh_rots_c;

    Matrix_TranslateRel16(pose != nullptr ? pose->offset : frame->offset);
    Matrix_Rot16(mesh_rots[LM_HIPS]);
    Output_DrawObjectMesh(lara->mesh_ptrs[LM_HIPS], clip);

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

    Matrix_TranslateRel32(bone[LM_TORSO - 1].pos);
    if (Lara_IsM16Active() && pose == nullptr) {
        mesh_rots =
            lara->right_arm.frame_base[lara->right_arm.frame_num].mesh_rots;
    }

    Matrix_Rot16(mesh_rots[LM_TORSO]);
    Matrix_Rot16(lara->interp.result.torso_rot);
    Output_DrawObjectMesh(lara->mesh_ptrs[LM_TORSO], clip);

    Matrix_Push();
    Matrix_TranslateRel32(bone[LM_HEAD - 1].pos);
    mesh_rots_c = mesh_rots;
    Matrix_Rot16(mesh_rots[LM_HEAD]);
    mesh_rots = mesh_rots_c;
    Matrix_Rot16(lara->interp.result.head_rot);
    Output_DrawObjectMesh(lara->mesh_ptrs[LM_HEAD], clip);

    *g_MatrixPtr = saved_matrix;
    Lara_Hair_Draw();

    Matrix_Pop();

#if TR_VERSION >= 2
    if (lara->back_gun_obj_id != O_LARA) {
        Matrix_Push();
        const OBJECT *const back_obj = Object_Get(lara->back_gun_obj_id);
        const ANIM_BONE *const bone_c = Object_GetBone(back_obj, 0);
        Matrix_TranslateRel32(bone_c[13].pos);
        mesh_rots_c = back_obj->frame_base->mesh_rots;
        Matrix_Rot16(mesh_rots_c[LM_HEAD]);
        Object_DrawMesh(back_obj->mesh_idx + LM_HEAD, clip, false);
        Matrix_Pop();
    }
#endif

    LARA_GUN_TYPE gun_type = LGT_UNARMED;
    if (pose == nullptr
        && (lara->gun_status == LGS_READY || lara->gun_status == LGS_SPECIAL
            || lara->gun_status == LGS_DRAW
            || lara->gun_status == LGS_UNDRAW)) {
        gun_type = lara->gun_type;
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
        Matrix_TranslateRel32(bone[LM_UARM_L - 1].pos);
#if TR_VERSION >= 2
        if (lara->flare.control && pose == nullptr) {
            const ANIM *const anim = Anim_GetAnim(lara->left_arm.anim_num);
            mesh_rots =
                lara->left_arm
                    .frame_base[lara->left_arm.frame_num - anim->frame_base]
                    .mesh_rots;
        }
#endif

        Matrix_Rot16(mesh_rots[LM_UARM_L]);
        Output_DrawObjectMesh(lara->mesh_ptrs[LM_UARM_L], clip);

        M_DrawBodyPart(LM_LARM_L, bone, mesh_rots, nullptr, clip);
        M_DrawBodyPart(LM_HAND_L, bone, mesh_rots, nullptr, clip);

#if TR_VERSION >= 2
        if (lara->gun_type == LGT_FLARE && lara->left_arm.flash_gun) {
            Gun_DrawFlash(LGT_FLARE, clip);
        }
#endif

        Matrix_Pop();
        break;

    case LGT_PISTOLS:
    case LGT_MAGNUMS:
    case LGT_UZIS: {
        Matrix_Push();
        Matrix_TranslateRel32(bone[LM_UARM_R - 1].pos);
        g_MatrixPtr->_00 = item_matrix._00;
        g_MatrixPtr->_01 = item_matrix._01;
        g_MatrixPtr->_02 = item_matrix._02;
        g_MatrixPtr->_10 = item_matrix._10;
        g_MatrixPtr->_11 = item_matrix._11;
        g_MatrixPtr->_12 = item_matrix._12;
        g_MatrixPtr->_20 = item_matrix._20;
        g_MatrixPtr->_21 = item_matrix._21;
        g_MatrixPtr->_22 = item_matrix._22;
        Matrix_Rot16(lara->right_arm.interp.result.rot);
        if (pose == nullptr) {
#if TR_VERSION == 1
            mesh_rots =
                lara->right_arm.frame_base[lara->right_arm.frame_num].mesh_rots;
#else
            const ANIM *const anim = Anim_GetAnim(lara->right_arm.anim_num);
            mesh_rots =
                lara->right_arm
                    .frame_base[lara->right_arm.frame_num - anim->frame_base]
                    .mesh_rots;
#endif
        }
        Matrix_Rot16(mesh_rots[LM_UARM_R]);
        Output_DrawObjectMesh(lara->mesh_ptrs[LM_UARM_R], clip);

        M_DrawBodyPart(LM_LARM_R, bone, mesh_rots, nullptr, clip);
        M_DrawBodyPart(LM_HAND_R, bone, mesh_rots, nullptr, clip);

        if (lara->right_arm.flash_gun) {
            saved_matrix = *g_MatrixPtr;
        }
        Matrix_Pop();

        Matrix_Push();
        Matrix_TranslateRel32(bone[LM_UARM_L - 1].pos);
        g_MatrixPtr->_00 = item_matrix._00;
        g_MatrixPtr->_01 = item_matrix._01;
        g_MatrixPtr->_02 = item_matrix._02;
        g_MatrixPtr->_10 = item_matrix._10;
        g_MatrixPtr->_11 = item_matrix._11;
        g_MatrixPtr->_12 = item_matrix._12;
        g_MatrixPtr->_20 = item_matrix._20;
        g_MatrixPtr->_21 = item_matrix._21;
        g_MatrixPtr->_22 = item_matrix._22;
        Matrix_Rot16(lara->left_arm.interp.result.rot);
        if (pose == nullptr) {
#if TR_VERSION == 1
            mesh_rots =
                lara->left_arm.frame_base[lara->left_arm.frame_num].mesh_rots;
#else
            const ANIM *const anim = Anim_GetAnim(lara->left_arm.anim_num);
            mesh_rots =
                lara->left_arm
                    .frame_base[lara->left_arm.frame_num - anim->frame_base]
                    .mesh_rots;
#endif
        }
        Matrix_Rot16(mesh_rots[LM_UARM_L]);
        Output_DrawObjectMesh(lara->mesh_ptrs[LM_UARM_L], clip);

        M_DrawBodyPart(LM_LARM_L, bone, mesh_rots, nullptr, clip);
        M_DrawBodyPart(LM_HAND_L, bone, mesh_rots, nullptr, clip);

        if (lara->left_arm.flash_gun) {
            Gun_DrawFlash(gun_type, clip);
        }
        if (lara->right_arm.flash_gun) {
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
        Matrix_TranslateRel32(bone[LM_UARM_R - 1].pos);
        if (pose == nullptr) {
            mesh_rots =
                lara->right_arm.frame_base[lara->right_arm.frame_num].mesh_rots;
        }
        Matrix_Rot16(mesh_rots[LM_UARM_R]);
        Output_DrawObjectMesh(lara->mesh_ptrs[LM_UARM_R], clip);

        M_DrawBodyPart(LM_LARM_R, bone, mesh_rots, nullptr, clip);
        M_DrawBodyPart(LM_HAND_R, bone, mesh_rots, nullptr, clip);

        if (lara->right_arm.flash_gun) {
            saved_matrix = *g_MatrixPtr;
        }
        Matrix_Pop();

        Matrix_Push();
        M_DrawBodyPart(LM_UARM_L, bone, mesh_rots, nullptr, clip);
        M_DrawBodyPart(LM_LARM_L, bone, mesh_rots, nullptr, clip);
        M_DrawBodyPart(LM_HAND_L, bone, mesh_rots, nullptr, clip);

        if (lara->right_arm.flash_gun) {
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
    g_PhdLeft = left;
    g_PhdRight = right;
    g_PhdTop = top;
    g_PhdBottom = bottom;
}
