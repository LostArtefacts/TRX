#include "game/objects/creatures/xian_common.h"

#include "game/items.h"
#include "game/objects/common.h"
#include "game/output.h"
#include "global/vars.h"

#include <libtrx/game/matrix.h>

// TODO: this duplicates Object_DrawAnimatingItem almost entirely
void XianWarrior_Draw(const ITEM *item)
{
    ANIM_FRAME *frames[2];
    int32_t rate;
    const int32_t frac = Item_GetFrames(item, frames, &rate);
    const OBJECT *const obj = Object_Get(item->object_id);

    if (obj->shadow_size != 0) {
        Output_InsertShadow(obj->shadow_size, &frames[0]->bounds, item);
    }

    Matrix_Push();
    Matrix_TranslateAbs32(item->interp.result.pos);
    Matrix_Rot16(item->interp.result.rot);

    const int32_t clip = Output_GetObjectBounds(&frames[0]->bounds);
    if (clip == 0) {
        Matrix_Pop();
        return;
    }

    Output_CalculateObjectLighting(item, &frames[0]->bounds);

    const OBJECT *jade_obj;
    if (item->object_id == O_XIAN_SPEARMAN) {
        jade_obj = Object_Get(O_XIAN_SPEARMAN_STATUE);
    } else {
        jade_obj = Object_Get(O_XIAN_KNIGHT_STATUE);
    }

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
            } else {
                Object_DrawMesh(jade_obj->mesh_idx + mesh_idx, clip, true);
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
            } else {
                Object_DrawMesh(jade_obj->mesh_idx + mesh_idx, clip, false);
            }
        }
    }

    Matrix_Pop();
}
