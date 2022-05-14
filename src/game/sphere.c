#include "game/sphere.h"

#include "game/draw.h"
#include "global/const.h"
#include "global/vars.h"
#include "math/matrix.h"

int32_t GetSpheres(ITEM_INFO *item, SPHERE *ptr, int32_t world_space)
{
    static int16_t null_rotation[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    if (!item) {
        return 0;
    }

    int32_t x;
    int32_t y;
    int32_t z;
    if (world_space) {
        x = item->pos.x;
        y = item->pos.y;
        z = item->pos.z;
        Matrix_PushUnit();
        g_MatrixPtr->_03 = 0;
        g_MatrixPtr->_13 = 0;
        g_MatrixPtr->_23 = 0;
    } else {
        x = 0;
        y = 0;
        z = 0;
        Matrix_Push();
        Matrix_TranslateAbs(item->pos.x, item->pos.y, item->pos.z);
    }

    Matrix_RotYXZ(item->pos.y_rot, item->pos.x_rot, item->pos.z_rot);

    int16_t *frame = GetBestFrame(item);
    Matrix_TranslateRel(
        frame[FRAME_POS_X], frame[FRAME_POS_Y], frame[FRAME_POS_Z]);

    int32_t *packed_rotation = (int32_t *)(frame + FRAME_ROT);
    Matrix_RotYXZpack(*packed_rotation++);

    OBJECT_INFO *object = &g_Objects[item->object_number];
    int16_t **meshpp = &g_Meshes[object->mesh_index];
    int32_t *bone = &g_AnimBones[object->bone_index];

    int16_t *objptr = *meshpp++;
    Matrix_Push();
    Matrix_TranslateRel(objptr[0], objptr[1], objptr[2]);
    ptr->x = x + (g_MatrixPtr->_03 >> W2V_SHIFT);
    ptr->y = y + (g_MatrixPtr->_13 >> W2V_SHIFT);
    ptr->z = z + (g_MatrixPtr->_23 >> W2V_SHIFT);
    ptr->r = objptr[3];
    ptr++;
    Matrix_Pop();

    int16_t *extra_rotation = item->data ? item->data : &null_rotation;
    for (int i = 1; i < object->nmeshes; i++) {
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

        objptr = *meshpp++;
        Matrix_Push();
        Matrix_TranslateRel(objptr[0], objptr[1], objptr[2]);
        ptr->x = x + (g_MatrixPtr->_03 >> W2V_SHIFT);
        ptr->y = y + (g_MatrixPtr->_13 >> W2V_SHIFT);
        ptr->z = z + (g_MatrixPtr->_23 >> W2V_SHIFT);
        ptr->r = objptr[3];
        Matrix_Pop();

        ptr++;
        bone += 4;
    }

    Matrix_Pop();
    return object->nmeshes;
}

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
