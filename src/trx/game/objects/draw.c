#include <trx/game/objects/draw.h>

#include <trx/debug.h>
#include <trx/game/anims/walk.h>
#include <trx/game/matrix.h>
#include <trx/game/objects.h>
#include <trx/game/output.h>

static bool M_DrawInterpolatedObject(
    const OBJECT *const obj, const uint32_t mesh_mask,
    const int16_t *extra_rotation, const ANIM_FRAME *const frame1,
    const ANIM_FRAME *const frame2, const int32_t frac, const int32_t rate,
    const OBJECT *const mesh_swap, const bool skips_frame_offset)
{
    if (frame1 == nullptr) {
        return false;
    }
    ASSERT(frame1 != nullptr);
    BOUNDS_16 bounds = frame1->bounds;
    if (skips_frame_offset) {
        const XYZ_16 offset = frame1->offset;
        bounds.min.x -= offset.x;
        bounds.max.x -= offset.x;
        bounds.min.y -= offset.y;
        bounds.max.y -= offset.y;
        bounds.min.z -= offset.z;
        bounds.max.z -= offset.z;
    }
    const CLIP clip = Object_GetClip(obj, &bounds);
    if (clip == CLIP_NOT_VISIBLE) {
        return false;
    }

    ASSERT(rate != 0);
    Matrix_Push();

    ANIM_WALK walk;
    Anim_Walk_Begin(
        &walk,
        &(ANIM_WALK_DESC) {
            .obj = obj,
            .pose = Anim_Pose_FromFrames(frame1, frame2, frac, rate),
            .extra_rotations = extra_rotation,
            .applies_base_rot = true,
            .skips_frame_offset = skips_frame_offset,
        });
    while (Anim_Walk_Next(&walk)) {
        const int32_t mesh_idx = walk.joint;
        if ((mesh_mask & (1u << mesh_idx)) != 0) {
            Object_DrawMesh(obj->mesh_idx + mesh_idx, clip, walk.interpolated);
        } else if (mesh_swap != nullptr) {
            Object_DrawMesh(
                mesh_swap->mesh_idx + mesh_idx, clip, walk.interpolated);
        }
    }
    Anim_Walk_End(&walk);

    Matrix_Pop();
    return true;
}

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

bool Object_DrawInterpolatedObject(
    const OBJECT *const obj, const uint32_t mesh_mask,
    const int16_t *extra_rotation, const ANIM_FRAME *const frame1,
    const ANIM_FRAME *const frame2, const int32_t frac, const int32_t rate)
{
    return M_DrawInterpolatedObject(
        obj, mesh_mask, extra_rotation, frame1, frame2, frac, rate, nullptr,
        false);
}

bool Object_DrawInterpolatedObjectWithSwap(
    const OBJECT *const obj, const uint32_t mesh_mask,
    const int16_t *const extra_rotation, const ANIM_FRAME *const frame1,
    const ANIM_FRAME *const frame2, const int32_t frac, const int32_t rate,
    const OBJECT *const mesh_swap)
{
    return M_DrawInterpolatedObject(
        obj, mesh_mask, extra_rotation, frame1, frame2, frac, rate, mesh_swap,
        false);
}

bool Object_DrawInterpolatedObjectAtPivot(
    const OBJECT *const obj, const uint32_t mesh_mask,
    const int16_t *const extra_rotation, const ANIM_FRAME *const frame1,
    const ANIM_FRAME *const frame2, const int32_t frac, const int32_t rate)
{
    return M_DrawInterpolatedObject(
        obj, mesh_mask, extra_rotation, frame1, frame2, frac, rate, nullptr,
        true);
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
