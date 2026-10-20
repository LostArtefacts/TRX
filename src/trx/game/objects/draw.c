#include <trx/game/objects/draw.h>

#include <trx/game/matrix.h>
#include <trx/game/objects.h>
#include <trx/game/output.h>

CLIP Object_GetClip(const OBJECT *const obj, const BOUNDS_16 *const bounds)
{
    return (obj->can_clip_func == nullptr || obj->can_clip_func())
        ? Output_CheckBoundsClip(bounds)
        : CLIP_FULLY_VISIBLE;
}

void Object_DrawMesh(
    const int32_t mesh_idx, const CLIP clip, const bool interpolated)
{
    const OBJECT_MESH *const mesh = Object_GetMesh(mesh_idx);
    if (interpolated) {
        Output_DrawObjectMesh_I(mesh, clip);
    } else {
        Output_DrawObjectMesh(mesh, clip);
    }
}

void Object_DrawStaticObject(
    const OBJECT *const obj, const ANIM_FRAME *const frame,
    const uint32_t mesh_mask)
{
    Matrix_Push();
    if ((mesh_mask & 1u) != 0) {
        Object_DrawMesh(obj->mesh_idx, 0, false);
    }
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
        if ((mesh_mask & (1u << i)) != 0) {
            Object_DrawMesh(obj->mesh_idx + i, 0, false);
        }
    }
    Matrix_Pop();
}

void Object_ApplyExtraRotation(
    const int16_t **const extra_rotation, const XYZ_BOOL rot_flags,
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
