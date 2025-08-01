#include "game/objects/draw.h"

#include "debug.h"
#include "game/matrix.h"
#include "game/objects/common.h"
#include "game/output.h"

void Object_DrawStaticObject(
    const OBJECT *const obj, const ANIM_FRAME *const frame)
{
    Matrix_Push();
    Object_DrawMesh(obj->mesh_idx, 0, false);
    for (int32_t i = 1; i < obj->mesh_count; i++) {
        const ANIM_BONE *const bone = Object_GetBone(obj, i - 1);
        if (bone->matrix_pop) {
            Matrix_Pop();
        }
        if (bone->matrix_push) {
            Matrix_Push();
        }

        Matrix_TranslateRel32(bone->pos);
        Matrix_Rot16(frame->mesh_rots[i]);
        Object_DrawMesh(obj->mesh_idx + i, 0, false);
    }
    Matrix_Pop();
}

void Object_DrawInterpolatedObject(
    const OBJECT *const obj, const uint32_t meshes,
    const int16_t *extra_rotation, const ANIM_FRAME *const frame1,
    const ANIM_FRAME *const frame2, const int32_t frac, const int32_t rate)
{
    if (frame1 == nullptr) {
        return;
    }
    ASSERT(frame1 != nullptr);
    const int32_t clip = Output_GetObjectBounds(&frame1->bounds);
    if (clip == 0) {
        return;
    }

    ASSERT(rate != 0);
    Matrix_Push();

    if (frac != 0) {
        for (int32_t mesh_idx = 0; mesh_idx < obj->mesh_count; mesh_idx++) {
            if (mesh_idx == 0) {
                Matrix_InitInterpolate(frac, rate);
                Matrix_TranslateRel16_ID(frame1->offset, frame2->offset);
                Matrix_Rot16_ID(
                    frame1->mesh_rots[mesh_idx], frame2->mesh_rots[mesh_idx]);
                Object_ApplyExtraRotation(&extra_rotation, obj->base_rot, true);
            } else {
                const ANIM_BONE *const bone = Object_GetBone(obj, mesh_idx - 1);
                if (bone->matrix_pop) {
                    Matrix_Pop_I();
                }
                if (bone->matrix_push) {
                    Matrix_Push_I();
                }

                Matrix_TranslateRel32_I(bone->pos);
                Matrix_Rot16_ID(
                    frame1->mesh_rots[mesh_idx], frame2->mesh_rots[mesh_idx]);
                Object_ApplyExtraRotation(&extra_rotation, bone->rot, true);
            }

            if (meshes & (1 << mesh_idx)) {
                Object_DrawMesh(obj->mesh_idx + mesh_idx, clip, true);
            }
        }
    } else {
        for (int32_t mesh_idx = 0; mesh_idx < obj->mesh_count; mesh_idx++) {
            if (mesh_idx == 0) {
                Matrix_TranslateRel16(frame1->offset);
                Matrix_Rot16(frame1->mesh_rots[mesh_idx]);
                Object_ApplyExtraRotation(
                    &extra_rotation, obj->base_rot, false);
            } else {
                const ANIM_BONE *const bone = Object_GetBone(obj, mesh_idx - 1);
                if (bone->matrix_pop) {
                    Matrix_Pop();
                }
                if (bone->matrix_push) {
                    Matrix_Push();
                }

                Matrix_TranslateRel32(bone->pos);
                Matrix_Rot16(frame1->mesh_rots[mesh_idx]);
                Object_ApplyExtraRotation(&extra_rotation, bone->rot, false);
            }

            if (meshes & (1 << mesh_idx)) {
                Object_DrawMesh(obj->mesh_idx + mesh_idx, clip, false);
            }
        }
    }

    Matrix_Pop();
}

void Object_ApplyExtraRotation(
    const int16_t **extra_rotation, const XYZ_BOOL rot_flags,
    const bool interpolated)
{
    const int16_t *rot_ptr = *extra_rotation;
    if (rot_ptr == nullptr) {
        return;
    }

#define APPLY_ROTATION(axis_, flag_)                                           \
    if (rot_flags.flag_) {                                                     \
        if (interpolated) {                                                    \
            Matrix_Rot##axis_##_I(*rot_ptr++);                                 \
        } else {                                                               \
            Matrix_Rot##axis_(*rot_ptr++);                                     \
        }                                                                      \
    }

    APPLY_ROTATION(Y, y);
    APPLY_ROTATION(X, x);
    APPLY_ROTATION(Z, z);

#undef APPLY_ROTATION
    *extra_rotation = rot_ptr;
}
