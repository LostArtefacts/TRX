#include "decomp/effects.h"

#include "game/effects.h"
#include "game/items.h"
#include "game/matrix.h"
#include "game/random.h"
#include "global/funcs.h"
#include "global/types.h"
#include "global/vars.h"

int32_t __cdecl Effect_ExplodingDeath(
    const int16_t item_num, const int32_t mesh_bits, const int16_t damage)
{
    ITEM *const item = &g_Items[item_num];
    const OBJECT *const object = &g_Objects[item->object_id];

    S_CalculateLight(item->pos.x, item->pos.y, item->pos.z, item->room_num);

    const FRAME_INFO *const best_frame = Item_GetBestFrame(item);

    Matrix_PushUnit();
    g_MatrixPtr->_03 = 0;
    g_MatrixPtr->_13 = 0;
    g_MatrixPtr->_23 = 0;
    g_MatrixPtr->_23 = 0;

    Matrix_RotYXZ(item->rot.y, item->rot.x, item->rot.z);
    Matrix_TranslateRel(
        best_frame->offset.x, best_frame->offset.y, best_frame->offset.z);

    const int16_t *mesh_rots = best_frame->mesh_rots;
    Matrix_RotYXZsuperpack(&mesh_rots, 0);

    // main mesh
    int32_t bit = 1;
    if ((mesh_bits & bit) && (item->mesh_bits & bit)) {
        const int16_t fx_num = Effect_Create(item->room_num);
        if (fx_num != NO_ITEM) {
            FX *const fx = &g_Effects[fx_num];
            fx->pos.x = item->pos.x + (g_MatrixPtr->_03 >> W2V_SHIFT);
            fx->pos.y = item->pos.y + (g_MatrixPtr->_13 >> W2V_SHIFT);
            fx->pos.z = item->pos.z + (g_MatrixPtr->_23 >> W2V_SHIFT);
            fx->rot.y = (Random_GetControl() - 0x4000) * 2;
            fx->room_num = item->room_num;
            fx->speed = Random_GetControl() >> 8;
            fx->fall_speed = -Random_GetControl() >> 8;
            fx->counter = damage;
            fx->object_id = O_BODY_PART;
            fx->frame_num = object->mesh_idx;
            fx->shade = g_LsAdder - 0x300;
        }
        item->mesh_bits &= ~bit;
    }

    // additional meshes
    const int32_t *bone = &g_AnimBones[object->bone_idx];
    const int16_t *extra_rotation = (int16_t *)item->data;
    for (int32_t i = 1; i < object->mesh_count; i++) {
        uint32_t bone_flags = *bone++;
        if (bone_flags & BF_MATRIX_POP) {
            Matrix_Pop();
        }
        if (bone_flags & BF_MATRIX_PUSH) {
            Matrix_Push();
        }

        Matrix_TranslateRel(bone[0], bone[1], bone[2]);
        Matrix_RotYXZsuperpack(&mesh_rots, 0);

        if (extra_rotation != NULL
            && bone_flags & (BF_ROT_X | BF_ROT_Y | BF_ROT_Z)) {
            if (bone_flags & BF_ROT_Y) {
                Matrix_RotY(*extra_rotation++);
            }
            if (bone_flags & BF_ROT_X) {
                Matrix_RotX(*extra_rotation++);
            }
            if (bone_flags & BF_ROT_Z) {
                Matrix_RotZ(*extra_rotation++);
            }
        }

        bit <<= 1;
        if ((mesh_bits & bit) && (item->mesh_bits & bit)) {
            const int16_t fx_num = Effect_Create(item->room_num);
            if (fx_num != NO_ITEM) {
                FX *const fx = &g_Effects[fx_num];
                fx->pos.x = item->pos.x + (g_MatrixPtr->_03 >> W2V_SHIFT);
                fx->pos.y = item->pos.y + (g_MatrixPtr->_13 >> W2V_SHIFT);
                fx->pos.z = item->pos.z + (g_MatrixPtr->_23 >> W2V_SHIFT);
                fx->rot.y = (Random_GetControl() - 0x4000) * 2;
                fx->room_num = item->room_num;
                fx->speed = Random_GetControl() >> 8;
                fx->fall_speed = -Random_GetControl() >> 8;
                fx->counter = damage;
                fx->object_id = O_BODY_PART;
                fx->frame_num = object->mesh_idx + i;
                fx->shade = g_LsAdder - 0x300;
            }
            item->mesh_bits &= ~bit;
        }

        bone += 3;
    }

    Matrix_Pop();

    return !(item->mesh_bits & (INT32_MAX >> (31 - object->mesh_count)));
}
