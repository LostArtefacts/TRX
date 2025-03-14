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

int32_t Collide_TestCollision(ITEM *item, const ITEM *lara_item)
{
    SPHERE slist_baddie[34];
    SPHERE slist_lara[34];

    uint32_t flags = 0;
    int32_t num1 = Collide_GetSpheres(item, slist_baddie, 1);
    int32_t num2 = Collide_GetSpheres(lara_item, slist_lara, 1);

    for (int i = 0; i < num1; i++) {
        SPHERE *ptr1 = &slist_baddie[i];
        if (ptr1->r <= 0) {
            continue;
        }
        for (int j = 0; j < num2; j++) {
            SPHERE *ptr2 = &slist_lara[j];
            if (ptr2->r <= 0) {
                continue;
            }
            int32_t x = ptr2->pos.x - ptr1->pos.x;
            int32_t y = ptr2->pos.y - ptr1->pos.y;
            int32_t z = ptr2->pos.z - ptr1->pos.z;
            int32_t r = ptr2->r + ptr1->r;
            int32_t d = SQUARE(x) + SQUARE(y) + SQUARE(z);
            int32_t r2 = SQUARE(r);
            if (d < r2) {
                flags |= 1 << i;
                break;
            }
        }
    }

    item->touch_bits = flags;
    return flags;
}

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
