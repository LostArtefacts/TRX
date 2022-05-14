#include "game/sphere.h"

#include "game/draw.h"
#include "global/const.h"
#include "global/vars.h"
#include "math/matrix.h"

void GetJointAbsPosition(ITEM_INFO *item, PHD_VECTOR *vec, int32_t joint)
{
    OBJECT_INFO *object = &g_Objects[item->object_number];

    Matrix_PushUnit();
    g_MatrixPtr->_03 = 0;
    g_MatrixPtr->_13 = 0;
    g_MatrixPtr->_23 = 0;

    Matrix_RotYXZ(item->pos.y_rot, item->pos.x_rot, item->pos.z_rot);

    int16_t *frame = GetBestFrame(item);
    Matrix_TranslateRel(
        frame[FRAME_POS_X], frame[FRAME_POS_Y], frame[FRAME_POS_Z]);

    int32_t *packed_rotation = (int32_t *)(frame + FRAME_ROT);
    Matrix_RotYXZpack(*packed_rotation++);

    int32_t *bone = &g_AnimBones[object->bone_index];

    int16_t *extra_rotation = (int16_t *)item->data;
    for (int i = 0; i < joint; i++) {
        int32_t bone_extra_flags = bone[0];
        if (bone_extra_flags & BEB_POP) {
            Matrix_Pop();
        }
        if (bone_extra_flags & BEB_PUSH) {
            Matrix_Push();
        }

        Matrix_TranslateRel(bone[1], bone[2], bone[3]);
        Matrix_RotYXZpack(*packed_rotation++);

        if (bone_extra_flags & BEB_ROT_Y) {
            Matrix_RotY(*extra_rotation++);
        }
        if (bone_extra_flags & BEB_ROT_X) {
            Matrix_RotX(*extra_rotation++);
        }
        if (bone_extra_flags & BEB_ROT_Z) {
            Matrix_RotZ(*extra_rotation++);
        }

        bone += 4;
    }

    Matrix_TranslateRel(vec->x, vec->y, vec->z);
    vec->x = (g_MatrixPtr->_03 >> W2V_SHIFT) + item->pos.x;
    vec->y = (g_MatrixPtr->_13 >> W2V_SHIFT) + item->pos.y;
    vec->z = (g_MatrixPtr->_23 >> W2V_SHIFT) + item->pos.z;
    Matrix_Pop();
}
