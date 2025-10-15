#include "game/objects/draw.h"

#include "config.h"
#include "debug.h"
#include "game/matrix.h"
#include "game/objects/common.h"
#include "game/output.h"
#include "game/output/vars.h"

void Object_DrawUnclippedItem(const ITEM *const item)
{
    const int32_t left = g_PhdLeft;
    const int32_t top = g_PhdTop;
    const int32_t right = g_PhdRight;
    const int32_t bottom = g_PhdBottom;

    g_PhdLeft = Viewport_GetMinX(VIEWPORT_GAME);
    g_PhdTop = Viewport_GetMinY(VIEWPORT_GAME);
    g_PhdRight = Viewport_GetMaxX(VIEWPORT_GAME);
    g_PhdBottom = Viewport_GetMaxY(VIEWPORT_GAME);

    Object_DrawAnimatingItem(item);

    g_PhdLeft = left;
    g_PhdTop = top;
    g_PhdRight = right;
    g_PhdBottom = bottom;
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

void Object_DrawAnimatingItem(const ITEM *item)
{
    ANIM_FRAME *frames[2];
    int32_t rate;
    int32_t frac = Item_GetFrames(item, frames, &rate);
    const OBJECT *const obj = Object_Get(item->object_id);

    if (obj->shadow_size != 0) {
        Output_DrawShadow(obj->shadow_size, &frames[0]->bounds, item);
    }

    Matrix_Push();
    Matrix_TranslateAbs32(item->interp.result.pos);
    Matrix_Rot16(item->interp.result.rot);

    const CLIP clip = Output_CheckBoundsClip(&frames[0]->bounds);
    if (clip == CLIP_NOT_VISIBLE) {
        Matrix_Pop();
        return;
    }

    Output_CalculateObjectLighting(item, &frames[0]->bounds);

    const int16_t *extra_rotation = item->data;

    Object_DrawInterpolatedObject(
        obj, item->mesh_bits, extra_rotation, frames[0], frames[1], frac, rate);
    if (g_Config.debug.enable_debug_cuboids) {
        Output_DrawCuboid(&frames[0]->bounds);
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
    const CLIP clip = Output_CheckBoundsClip(&frame1->bounds);
    if (clip == CLIP_NOT_VISIBLE) {
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
