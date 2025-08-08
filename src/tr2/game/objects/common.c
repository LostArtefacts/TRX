#include "game/objects/common.h"

#include "game/viewport.h"
#include "global/vars.h"

#include <libtrx/debug.h>
#include <libtrx/game/collision.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/matrix.h>
#include <libtrx/game/output.h>
#include <libtrx/utils.h>

void Object_DrawDummyItem(const ITEM *const item)
{
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
    Matrix_Pop();
}

void Object_DrawUnclippedItem(const ITEM *const item)
{
    int32_t left = g_PhdLeft;
    int32_t top = g_PhdTop;
    int32_t right = g_PhdRight;
    int32_t bottom = g_PhdBottom;

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

void Object_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);

    if (!Item_TestBoundsCollide(item, lara_item, coll->radius)) {
        return;
    }

    if (!Collide_TestCollision(item, lara_item)) {
        return;
    }

    if (coll->enable_baddie_push) {
        Lara_Push(item, coll, false, true);
    }
}

void Object_Collision_Trap(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);

    if (item->status == IS_ACTIVE) {
        if (Item_TestBoundsCollide(item, lara_item, coll->radius)) {
            Collide_TestCollision(item, lara_item);
        }
    } else if (item->status != IS_INVISIBLE) {
        Object_Collision(item_num, lara_item, coll);
    }
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

void Object_SetReflective(const GAME_OBJECT_ID obj_id, const bool enabled)
{
    ASSERT_FAIL();
}
