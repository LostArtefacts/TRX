#include "game/collide.h"

#include "game/items.h"
#include "game/output.h"
#include "game/room.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/math.h>
#include <libtrx/game/matrix.h>
#include <libtrx/utils.h>

void Collide_GetJointAbsPosition(ITEM *item, XYZ_32 *vec, int32_t joint)
{
    const OBJECT *const obj = Object_Get(item->object_id);

    Matrix_PushUnit();
    Matrix_Rot16(item->rot);

    const ANIM_FRAME *const frame = Item_GetBestFrame(item);
    Matrix_TranslateRel16(frame->offset);
    Matrix_Rot16(frame->mesh_rots[0]);

    int16_t *extra_rotation = (int16_t *)item->data;
    const int32_t abs_joint = MIN(obj->mesh_count, joint);
    for (int32_t i = 0; i < abs_joint; i++) {
        const ANIM_BONE *const bone = Object_GetBone(obj, i);
        if (bone->matrix_pop) {
            Matrix_Pop();
        }
        if (bone->matrix_push) {
            Matrix_Push();
        }

        Matrix_TranslateRel32(bone->pos);
        Matrix_Rot16(frame->mesh_rots[i + 1]);

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

    Matrix_TranslateRel32(*vec);
    vec->x = (g_MatrixPtr->_03 >> W2V_SHIFT) + item->pos.x;
    vec->y = (g_MatrixPtr->_13 >> W2V_SHIFT) + item->pos.y;
    vec->z = (g_MatrixPtr->_23 >> W2V_SHIFT) + item->pos.z;
    Matrix_Pop();
}
