#include "game/draw.h"

#include "game/items.h"
#include "game/output.h"
#include "game/viewport.h"
#include "global/const.h"
#include "global/vars.h"
#include "math/matrix.h"

#include <stdint.h>

void DrawAnimatingItem(ITEM_INFO *item)
{
    static int16_t null_rotation[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    int16_t *frmptr[2];
    int32_t rate;
    int32_t frac = Item_GetFrames(item, frmptr, &rate);
    OBJECT_INFO *object = &g_Objects[item->object_number];

    if (object->shadow_size) {
        Output_DrawShadow(object->shadow_size, frmptr[0], item);
    }

    Matrix_Push();
    Matrix_TranslateAbs(item->pos.x, item->pos.y, item->pos.z);
    Matrix_RotYXZ(item->pos.y_rot, item->pos.x_rot, item->pos.z_rot);
    int32_t clip = Output_GetObjectBounds(frmptr[0]);
    if (!clip) {
        Matrix_Pop();
        return;
    }

    Output_CalculateObjectLighting(item, frmptr[0]);
    int16_t *extra_rotation = item->data ? item->data : &null_rotation;

    int32_t bit = 1;
    int16_t **meshpp = &g_Meshes[object->mesh_index];
    int32_t *bone = &g_AnimBones[object->bone_index];

    if (!frac) {
        Matrix_TranslateRel(
            frmptr[0][FRAME_POS_X], frmptr[0][FRAME_POS_Y],
            frmptr[0][FRAME_POS_Z]);

        int32_t *packed_rotation = (int32_t *)(frmptr[0] + FRAME_ROT);
        Matrix_RotYXZpack(*packed_rotation++);

        if (item->mesh_bits & bit) {
            Output_DrawPolygons(*meshpp++, clip);
        }

        for (int i = 1; i < object->nmeshes; i++) {
            int32_t bone_extra_flags = *bone;
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

            bit <<= 1;
            if (item->mesh_bits & bit) {
                Output_DrawPolygons(*meshpp, clip);
            }

            bone += 4;
            meshpp++;
        }
    } else {
        Matrix_InitInterpolate(frac, rate);
        Matrix_TranslateRel_ID(
            frmptr[0][FRAME_POS_X], frmptr[0][FRAME_POS_Y],
            frmptr[0][FRAME_POS_Z], frmptr[1][FRAME_POS_X],
            frmptr[1][FRAME_POS_Y], frmptr[1][FRAME_POS_Z]);
        int32_t *packed_rotation1 = (int32_t *)(frmptr[0] + FRAME_ROT);
        int32_t *packed_rotation2 = (int32_t *)(frmptr[1] + FRAME_ROT);
        Matrix_RotYXZpack_I(*packed_rotation1++, *packed_rotation2++);

        if (item->mesh_bits & bit) {
            Output_DrawPolygons_I(*meshpp++, clip);
        }

        for (int i = 1; i < object->nmeshes; i++) {
            int32_t bone_extra_flags = *bone;
            if (bone_extra_flags & BEB_POP) {
                Matrix_Pop_I();
            }

            if (bone_extra_flags & BEB_PUSH) {
                Matrix_Push_I();
            }

            Matrix_TranslateRel_I(bone[1], bone[2], bone[3]);
            Matrix_RotYXZpack_I(*packed_rotation1++, *packed_rotation2++);

            if (bone_extra_flags & BEB_ROT_Y) {
                Matrix_RotY_I(*extra_rotation++);
            }
            if (bone_extra_flags & BEB_ROT_X) {
                Matrix_RotX_I(*extra_rotation++);
            }
            if (bone_extra_flags & BEB_ROT_Z) {
                Matrix_RotZ_I(*extra_rotation++);
            }

            bit <<= 1;
            if (item->mesh_bits & bit) {
                Output_DrawPolygons_I(*meshpp, clip);
            }

            bone += 4;
            meshpp++;
        }
    }

    Matrix_Pop();
}

void DrawUnclippedItem(ITEM_INFO *item)
{
    int32_t left = g_PhdLeft;
    int32_t top = g_PhdTop;
    int32_t right = g_PhdRight;
    int32_t bottom = g_PhdBottom;

    g_PhdLeft = Viewport_GetMinX();
    g_PhdTop = Viewport_GetMinY();
    g_PhdRight = Viewport_GetMaxX();
    g_PhdBottom = Viewport_GetMaxY();

    DrawAnimatingItem(item);

    g_PhdLeft = left;
    g_PhdTop = top;
    g_PhdRight = right;
    g_PhdBottom = bottom;
}
