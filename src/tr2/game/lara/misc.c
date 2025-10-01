#include "game/lara/misc.h"

#include <libtrx/game/lara.h>
#include <libtrx/game/matrix.h>

static void M_GetJointAbsPosition_I(
    XYZ_32 *const vec, const ANIM_FRAME *const frame1,
    const ANIM_FRAME *const frame2, const int32_t frac, const int32_t rate)
{
    const LARA_INFO *const lara_info = Lara_GetLaraInfo();
    const ITEM *const item = Lara_GetItem();
    const OBJECT *obj = Object_Get(item->object_id);

    Matrix_PushUnit();
    Matrix_Rot16(item->rot);

    const ANIM_BONE *const bone = Object_GetBone(obj, 0);
    const XYZ_16 *mesh_rots_1 = frame1->mesh_rots;
    const XYZ_16 *mesh_rots_2 = frame2->mesh_rots;
    Matrix_InitInterpolate(frac, rate);

    Matrix_TranslateRel16_ID(frame1->offset, frame2->offset);
    Matrix_Rot16_ID(mesh_rots_1[LM_HIPS], mesh_rots_2[LM_HIPS]);

    Matrix_TranslateRel32_I(bone[LM_TORSO - 1].pos);
    Matrix_Rot16_ID(mesh_rots_1[LM_TORSO], mesh_rots_2[LM_TORSO]);
    Matrix_Rot16_I(lara_info->torso_rot);

    LARA_GUN_TYPE gun_type = LGT_UNARMED;
    if (lara_info->gun_status == LGS_READY
        || lara_info->gun_status == LGS_SPECIAL
        || lara_info->gun_status == LGS_DRAW
        || lara_info->gun_status == LGS_UNDRAW) {
        gun_type = lara_info->gun_type;
    }

    if (lara_info->gun_type == LGT_FLARE) {
        Matrix_Interpolate();
        Matrix_TranslateRel32(bone[LM_UARM_L - 1].pos);
        if (lara_info->flare.control) {
            const LARA_ARM *const arm = &lara_info->left_arm;
            const ANIM *const anim = Anim_GetAnim(arm->anim_num);
            mesh_rots_1 =
                arm->frame_base[arm->frame_num - anim->frame_base].mesh_rots;
        } else {
            mesh_rots_1 = frame1->mesh_rots;
        }
        Matrix_Rot16(mesh_rots_1[LM_UARM_L]);

        Matrix_TranslateRel32(bone[LM_LARM_L - 1].pos);
        Matrix_Rot16(mesh_rots_1[LM_LARM_L]);

        Matrix_TranslateRel32(bone[LM_HAND_L - 1].pos);
        Matrix_Rot16(mesh_rots_1[LM_HAND_L]);
    } else if (gun_type != LGT_UNARMED) {
        Matrix_Interpolate();
        Matrix_TranslateRel32(bone[LM_UARM_R - 1].pos);

        const LARA_ARM *const arm = &lara_info->right_arm;
        const ANIM *const anim = Anim_GetAnim(arm->anim_num);
        mesh_rots_1 = arm->frame_base[arm->frame_num].mesh_rots;
        Matrix_Rot16(mesh_rots_1[LM_UARM_R]);

        Matrix_TranslateRel32(bone[LM_LARM_R - 1].pos);
        Matrix_Rot16(mesh_rots_1[LM_LARM_R]);

        Matrix_TranslateRel32(bone[LM_HAND_R - 1].pos);
        Matrix_Rot16(mesh_rots_1[LM_HAND_R]);
    }

    Matrix_TranslateRel32(*vec);
    vec->x = item->pos.x + (g_MatrixPtr->_03 >> W2V_SHIFT);
    vec->y = item->pos.y + (g_MatrixPtr->_13 >> W2V_SHIFT);
    vec->z = item->pos.z + (g_MatrixPtr->_23 >> W2V_SHIFT);
    Matrix_Pop();
}

// TODO: joint is ignored - this only works for hands.
void Lara_GetJointAbsPosition(XYZ_32 *const vec, const LARA_MESH joint)
{
    const LARA_INFO *const lara_info = Lara_GetLaraInfo();
    const ITEM *const lara_item = Lara_GetItem();
    ANIM_FRAME *frmptr[2] = { nullptr, nullptr };
    if (lara_info->hit_direction < 0) {
        int32_t rate;
        const int32_t frac = Item_GetFrames(lara_item, frmptr, &rate);
        if (frac != 0) {
            M_GetJointAbsPosition_I(vec, frmptr[0], frmptr[1], frac, rate);
            return;
        }
    }

    const ANIM_FRAME *const hit_frame = Lara_GetHitFrame(lara_item);
    const ANIM_FRAME *const frame_ptr =
        hit_frame == nullptr ? frmptr[0] : hit_frame;

    Matrix_PushUnit();
    Matrix_Rot16(lara_item->rot);

    const XYZ_16 *mesh_rots = frame_ptr->mesh_rots;
    const OBJECT *const obj = Object_Get(lara_item->object_id);
    const ANIM_BONE *bone = Object_GetBone(obj, 0);

    Matrix_TranslateRel16(frame_ptr->offset);
    Matrix_Rot16(mesh_rots[LM_HIPS]);

    Matrix_TranslateRel32(bone[LM_TORSO - 1].pos);
    Matrix_Rot16(mesh_rots[LM_TORSO]);
    Matrix_Rot16(lara_info->torso_rot);

    LARA_GUN_TYPE gun_type = LGT_UNARMED;
    if (lara_info->gun_status == LGS_READY
        || lara_info->gun_status == LGS_SPECIAL
        || lara_info->gun_status == LGS_DRAW
        || lara_info->gun_status == LGS_UNDRAW) {
        gun_type = lara_info->gun_type;
    }

    if (lara_info->gun_type == LGT_FLARE) {
        Matrix_TranslateRel32(bone[LM_UARM_L - 1].pos);
        if (lara_info->flare.control) {
            const LARA_ARM *const arm = &lara_info->left_arm;
            const ANIM *const anim = Anim_GetAnim(arm->anim_num);
            mesh_rots =
                arm->frame_base[arm->frame_num - anim->frame_base].mesh_rots;
        } else {
            mesh_rots = frame_ptr->mesh_rots;
        }
        Matrix_Rot16(mesh_rots[LM_UARM_L]);

        Matrix_TranslateRel32(bone[LM_LARM_L - 1].pos);
        Matrix_Rot16(mesh_rots[LM_LARM_L]);

        Matrix_TranslateRel32(bone[LM_HAND_L - 1].pos);
        Matrix_Rot16(mesh_rots[LM_HAND_L]);
    } else if (gun_type != LGT_UNARMED) {
        Matrix_TranslateRel32(bone[LM_UARM_R - 1].pos);

        const LARA_ARM *const arm = &lara_info->right_arm;
        const ANIM *const anim = Anim_GetAnim(arm->anim_num);
        mesh_rots = arm->frame_base[arm->frame_num].mesh_rots;
        Matrix_Rot16(mesh_rots[LM_UARM_R]);

        Matrix_TranslateRel32(bone[LM_LARM_R - 1].pos);
        Matrix_Rot16(mesh_rots[LM_LARM_R]);

        Matrix_TranslateRel32(bone[LM_HAND_R - 1].pos);
        Matrix_Rot16(mesh_rots[LM_HAND_R]);
    }

    Matrix_TranslateRel32(*vec);
    vec->x = lara_item->pos.x + (g_MatrixPtr->_03 >> W2V_SHIFT);
    vec->y = lara_item->pos.y + (g_MatrixPtr->_13 >> W2V_SHIFT);
    vec->z = lara_item->pos.z + (g_MatrixPtr->_23 >> W2V_SHIFT);
    Matrix_Pop();
}
