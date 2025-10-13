#include "game/objects/common.h"

#include "game/inventory.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/game/matrix.h>
#include <libtrx/game/output.h>

void Object_DrawSpriteItem(const ITEM *const item)
{
    SHADE shade = item->shade;
    if (shade.value_1 < 0) {
        shade.value_1 = SHADE_NEUTRAL;
    }
    Output_CalculateStaticMeshLight(
        item->interp.result.pos, shade, Room_Get(item->room_num));

    const OBJECT *const obj = Object_Get(item->object_id);

    Output_DrawSprite(
        item->interp.result.pos.x, item->interp.result.pos.y,
        item->interp.result.pos.z, obj->mesh_idx - item->frame_num,
        Output_GetLightAdder() + SHADE_NEUTRAL, (RGB_F) { 1.0f, 1.0f, 1.0f });
}

void Object_DrawPickupItem(const ITEM *const item)
{
    if (item->flags & IF_INVISIBLE) {
        return;
    }

    if (!g_Config.visuals.enable_3d_pickups
        || !Object_Get(item->object_id)->loaded) {
        Object_DrawSpriteItem(item);
        return;
    }

    // Convert item to menu display item.
    const OBJECT_ID inv_object_id = Inv_GetItemOption(item->object_id);
    if (inv_object_id == NO_OBJECT) {
        Object_DrawSpriteItem(item);
        return;
    }

    const OBJECT *const obj = Object_Get(inv_object_id);
    if (!obj->loaded || obj->mesh_count < 0) {
        Object_DrawSpriteItem(item);
        return;
    }

    // Get the first frame of the first animation, and its bounding box.
    int16_t offset;
    BOUNDS_16 bounds;
    const ANIM_FRAME *frame = nullptr;

    // Some items, such as the Prayer Wheel in Barkhang Monastery, do not have
    // animations, and for such items we need to calculate this information
    // manually.
    if (obj->anim_idx != -1) {
        frame = obj->frame_base;
        bounds = frame->bounds;
        const int16_t y_off = frame->offset.y - bounds.max.y;
        bounds.max.y -= bounds.max.y;
        bounds.min.y -= bounds.max.y;
        offset = item->interp.result.pos.y + y_off;
    } else {
        bounds = Object_GetBoundingBox(obj, nullptr, item->mesh_bits);
        offset = item->pos.y - (bounds.max.y - bounds.min.y) / 2;
    }

    Matrix_Push();
    Matrix_TranslateAbs(
        item->interp.result.pos.x, offset, item->interp.result.pos.z);
    Matrix_Rot16(item->interp.result.rot);

    Output_CalculateLight(item->pos, item->room_num);

    const CLIP clip = Output_CheckBoundsClip(&bounds);
    if (clip != CLIP_NOT_VISIBLE) {
        int32_t bit = 1;

        const XYZ_16 *const mesh_rots =
            frame != nullptr ? frame->mesh_rots : nullptr;
        if (mesh_rots != nullptr) {
            Matrix_Rot16(mesh_rots[0]);
        }

        if (item->mesh_bits & bit) {
            Object_DrawMesh(obj->mesh_idx, clip, false);
        }

        for (int i = 1; i < obj->mesh_count; i++) {
            const ANIM_BONE *const bone = Object_GetBone(obj, i - 1);
            if (bone->matrix_pop) {
                Matrix_Pop();
            }

            if (bone->matrix_push) {
                Matrix_Push();
            }

            Matrix_TranslateRel32(bone->pos);
            if (mesh_rots != nullptr) {
                Matrix_Rot16(mesh_rots[i]);
            }

            // Extra rotation is ignored in this case as it's not needed.

            bit <<= 1;
            if (item->mesh_bits & bit) {
                Object_DrawMesh(obj->mesh_idx + i, clip, false);
            }
        }
    }

    Matrix_Pop();
}

BOUNDS_16 Object_GetBoundingBox(
    const OBJECT *const obj, const ANIM_FRAME *const frame,
    const uint32_t mesh_bits)
{
    const XYZ_16 *const mesh_rots =
        frame != nullptr ? frame->mesh_rots : nullptr;

    Matrix_PushUnit();
    if (frame != nullptr) {
        Matrix_TranslateRel16(frame->offset);
    }
    if (mesh_rots != nullptr) {
        Matrix_Rot16(mesh_rots[0]);
    }

    BOUNDS_16 new_bounds = {
        .min.x = 0x7FFF,
        .min.y = 0x7FFF,
        .min.z = 0x7FFF,
        .max.x = -0x7FFF,
        .max.y = -0x7FFF,
        .max.z = -0x7FFF,
    };

    for (int32_t mesh_idx = 0; mesh_idx < obj->mesh_count; mesh_idx++) {
        if (mesh_idx != 0) {
            const ANIM_BONE *const bone = Object_GetBone(obj, mesh_idx - 1);
            if (bone->matrix_pop) {
                Matrix_Pop();
            }

            if (bone->matrix_push) {
                Matrix_Push();
            }

            Matrix_TranslateRel32(bone->pos);
            if (mesh_rots != nullptr) {
                Matrix_Rot16(mesh_rots[mesh_idx]);
            }
        }

        if (!(mesh_bits & (1 << mesh_idx))) {
            continue;
        }

        const OBJECT_MESH *const mesh =
            Object_GetMesh(obj->mesh_idx + mesh_idx);
        for (int32_t i = 0; i < mesh->num_vertices; i++) {
            // clang-format off
            const XYZ_16 *const vertex = &mesh->vertices[i];
            const MATRIX *const mptr = g_MatrixPtr;
            const double xv = (
                mptr->_00 * vertex->x +
                mptr->_01 * vertex->y +
                mptr->_02 * vertex->z +
                mptr->_03
            );
            const double yv = (
                mptr->_10 * vertex->x +
                mptr->_11 * vertex->y +
                mptr->_12 * vertex->z +
                mptr->_13
            );
            double zv = (
                mptr->_20 * vertex->x +
                mptr->_21 * vertex->y +
                mptr->_22 * vertex->z +
                mptr->_23
            );
            // clang-format on

            const int32_t x = ((int32_t)xv) >> W2V_SHIFT;
            const int32_t y = ((int32_t)yv) >> W2V_SHIFT;
            const int32_t z = ((int32_t)zv) >> W2V_SHIFT;

            new_bounds.min.x = MIN(new_bounds.min.x, x);
            new_bounds.min.y = MIN(new_bounds.min.y, y);
            new_bounds.min.z = MIN(new_bounds.min.z, z);
            new_bounds.max.x = MAX(new_bounds.max.x, x);
            new_bounds.max.y = MAX(new_bounds.max.y, y);
            new_bounds.max.z = MAX(new_bounds.max.z, z);
        }
    }

    Matrix_Pop();
    return new_bounds;
}

void Object_SetReflective(const OBJECT_ID obj_id, const bool enabled)
{
    ASSERT_FAIL();
}
