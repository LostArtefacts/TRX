#include "game/lara/misc.h"

#include "game/effects.h"
#include "game/inventory_ring.h"
#include "game/lara/control.h"
#include "game/sound.h"
#include "global/vars.h"

#include <libtrx/game/lara.h>
#include <libtrx/game/matrix.h>
#include <libtrx/utils.h>

void Lara_GetJointAbsPosition(XYZ_32 *vec, int32_t joint)
{
    ANIM_FRAME *frmptr[2] = { nullptr, nullptr };
    if (g_Lara.hit_direction < 0) {
        int32_t rate;
        int32_t frac = Item_GetFrames(g_LaraItem, frmptr, &rate);
        if (frac) {
            Lara_GetJointAbsPosition_I(
                g_LaraItem, vec, frmptr[0], frmptr[1], frac, rate);
            return;
        }
    }

    const ANIM_FRAME *const hit_frame = Lara_GetHitFrame(g_LaraItem);
    const ANIM_FRAME *const frame_ptr =
        hit_frame == nullptr ? frmptr[0] : hit_frame;

    Matrix_PushUnit();
    g_MatrixPtr->_03 = 0;
    g_MatrixPtr->_13 = 0;
    g_MatrixPtr->_23 = 0;
    Matrix_Rot16(g_LaraItem->rot);

    const XYZ_16 *mesh_rots = frame_ptr->mesh_rots;
    const OBJECT *const obj = Object_Get(g_LaraItem->object_id);
    const ANIM_BONE *bone = Object_GetBone(obj, 0);

    Matrix_TranslateRel16(frame_ptr->offset);
    Matrix_Rot16(mesh_rots[LM_HIPS]);

    Matrix_TranslateRel32(bone[LM_TORSO - 1].pos);
    Matrix_Rot16(mesh_rots[LM_TORSO]);
    Matrix_Rot16(g_Lara.torso_rot);

    LARA_GUN_TYPE gun_type = LGT_UNARMED;
    if (g_Lara.gun_status == LGS_READY || g_Lara.gun_status == LGS_SPECIAL
        || g_Lara.gun_status == LGS_DRAW || g_Lara.gun_status == LGS_UNDRAW) {
        gun_type = g_Lara.gun_type;
    }

    if (g_Lara.gun_type == LGT_FLARE) {
        Matrix_TranslateRel32(bone[LM_UARM_L - 1].pos);
        if (g_Lara.flare.control) {
            const LARA_ARM *const arm = &g_Lara.left_arm;
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

        const LARA_ARM *const arm = &g_Lara.right_arm;
        const ANIM *const anim = Anim_GetAnim(arm->anim_num);
        mesh_rots = arm->frame_base[arm->frame_num].mesh_rots;
        Matrix_Rot16(mesh_rots[LM_UARM_R]);

        Matrix_TranslateRel32(bone[LM_LARM_R - 1].pos);
        Matrix_Rot16(mesh_rots[LM_LARM_R]);

        Matrix_TranslateRel32(bone[LM_HAND_R - 1].pos);
        Matrix_Rot16(mesh_rots[LM_HAND_R]);
    }

    Matrix_TranslateRel32(*vec);
    vec->x = g_LaraItem->pos.x + (g_MatrixPtr->_03 >> W2V_SHIFT);
    vec->y = g_LaraItem->pos.y + (g_MatrixPtr->_13 >> W2V_SHIFT);
    vec->z = g_LaraItem->pos.z + (g_MatrixPtr->_23 >> W2V_SHIFT);
    Matrix_Pop();
}

void Lara_GetJointAbsPosition_I(
    ITEM *item, XYZ_32 *vec, ANIM_FRAME *frame1, ANIM_FRAME *frame2,
    int32_t frac, int32_t rate)
{
    const OBJECT *obj = Object_Get(item->object_id);

    Matrix_PushUnit();
    g_MatrixPtr->_03 = 0;
    g_MatrixPtr->_13 = 0;
    g_MatrixPtr->_23 = 0;
    Matrix_Rot16(item->rot);

    const ANIM_BONE *const bone = Object_GetBone(obj, 0);
    const XYZ_16 *mesh_rots_1 = frame1->mesh_rots;
    const XYZ_16 *mesh_rots_2 = frame2->mesh_rots;
    Matrix_InitInterpolate(frac, rate);

    Matrix_TranslateRel16_ID(frame1->offset, frame2->offset);
    Matrix_Rot16_ID(mesh_rots_1[LM_HIPS], mesh_rots_2[LM_HIPS]);

    Matrix_TranslateRel32_I(bone[LM_TORSO - 1].pos);
    Matrix_Rot16_ID(mesh_rots_1[LM_TORSO], mesh_rots_2[LM_TORSO]);
    Matrix_Rot16_I(g_Lara.torso_rot);

    LARA_GUN_TYPE gun_type = LGT_UNARMED;
    if (g_Lara.gun_status == LGS_READY || g_Lara.gun_status == LGS_SPECIAL
        || g_Lara.gun_status == LGS_DRAW || g_Lara.gun_status == LGS_UNDRAW) {
        gun_type = g_Lara.gun_type;
    }

    if (g_Lara.gun_type == LGT_FLARE) {
        Matrix_Interpolate();
        Matrix_TranslateRel32(bone[LM_UARM_L - 1].pos);
        if (g_Lara.flare.control) {
            const LARA_ARM *const arm = &g_Lara.left_arm;
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

        const LARA_ARM *const arm = &g_Lara.right_arm;
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
