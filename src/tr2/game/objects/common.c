#include "game/objects/common.h"

#include "game/collide.h"
#include "game/items.h"
#include "game/lara/misc.h"
#include "game/output.h"
#include "game/room.h"
#include "game/viewport.h"
#include "global/vars.h"

#include <libtrx/game/matrix.h>
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
        Output_InsertShadow(obj->shadow_size, &frames[0]->bounds, item);
    }

    Matrix_Push();
    Matrix_TranslateAbs32(item->interp.result.pos);
    Matrix_Rot16(item->interp.result.rot);

    const int32_t clip = Output_GetObjectBounds(&frames[0]->bounds);
    if (!clip) {
        Matrix_Pop();
        return;
    }

    Output_CalculateObjectLighting(item, &frames[0]->bounds);

    const int16_t *extra_rotation = item->data;

    if (frac != 0) {
        for (int32_t mesh_idx = 0; mesh_idx < obj->mesh_count; mesh_idx++) {
            if (mesh_idx == 0) {
                Matrix_InitInterpolate(frac, rate);
                Matrix_TranslateRel16_ID(frames[0]->offset, frames[1]->offset);
                Matrix_Rot16_ID(
                    frames[0]->mesh_rots[mesh_idx],
                    frames[1]->mesh_rots[mesh_idx]);
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
                    frames[0]->mesh_rots[mesh_idx],
                    frames[1]->mesh_rots[mesh_idx]);
                if (extra_rotation != nullptr) {
                    if (bone->rot_y) {
                        Matrix_RotY_I(*extra_rotation++);
                    }
                    if (bone->rot_x) {
                        Matrix_RotX_I(*extra_rotation++);
                    }
                    if (bone->rot_z) {
                        Matrix_RotZ_I(*extra_rotation++);
                    }
                }
            }

            if (item->mesh_bits & (1 << mesh_idx)) {
                Object_DrawMesh(obj->mesh_idx + mesh_idx, clip, true);
            }
        }
    } else {
        for (int32_t mesh_idx = 0; mesh_idx < obj->mesh_count; mesh_idx++) {
            if (mesh_idx == 0) {
                Matrix_TranslateRel16(frames[0]->offset);
                Matrix_Rot16(frames[0]->mesh_rots[mesh_idx]);
            } else {
                const ANIM_BONE *const bone = Object_GetBone(obj, mesh_idx - 1);
                if (bone->matrix_pop) {
                    Matrix_Pop();
                }
                if (bone->matrix_push) {
                    Matrix_Push();
                }

                Matrix_TranslateRel32(bone->pos);
                Matrix_Rot16(frames[0]->mesh_rots[mesh_idx]);
                if (extra_rotation != nullptr) {
                    if (bone->rot_y) {
                        Matrix_RotY(*extra_rotation++);
                    }
                    if (bone->rot_x) {
                        Matrix_RotX(*extra_rotation++);
                    }
                    if (bone->rot_z) {
                        Matrix_RotZ(*extra_rotation++);
                    }
                }
            }

            if (item->mesh_bits & (1 << mesh_idx)) {
                Object_DrawMesh(obj->mesh_idx + mesh_idx, clip, false);
            }
        }
    }

    Matrix_Pop();
}

void Object_DrawUnclippedItem(const ITEM *const item)
{
    const VIEWPORT old_vp = *Viewport_Get();

    VIEWPORT new_vp = old_vp;
    new_vp.game_vars.win_top = 0;
    new_vp.game_vars.win_left = 0;
    new_vp.game_vars.win_bottom = new_vp.game_vars.win_max_y;
    new_vp.game_vars.win_right = new_vp.game_vars.win_max_x;

    Viewport_Restore(&new_vp);
    Object_DrawAnimatingItem(item);
    Viewport_Restore(&old_vp);
}

void Object_DrawSpriteItem(const ITEM *const item)
{
    Output_CalculateStaticMeshLight(
        item->interp.result.pos, item->shade, Room_Get(item->room_num));

    const OBJECT *const obj = Object_Get(item->object_id);

    Output_DrawSprite(
        SPRITE_ABS | (obj->semi_transparent ? SPRITE_SEMI_TRANS : 0)
            | SPRITE_SHADE,
        item->interp.result.pos.x, item->interp.result.pos.y,
        item->interp.result.pos.z, obj->mesh_idx - item->frame_num,
        Output_GetLightAdder() + 4096, 0);
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
        Lara_Push(item, lara_item, coll, false, true);
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
    const int32_t mesh_idx, const int32_t clip, const bool interpolated)
{
    const OBJECT_MESH *const mesh = Object_GetMesh(mesh_idx);
    if (interpolated) {
        Output_DrawObjectMesh_I(mesh, clip);
    } else {
        Output_DrawObjectMesh(mesh, clip);
    }
}
