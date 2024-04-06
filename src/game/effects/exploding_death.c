#include "game/effects/exploding_death.h"

#include "game/effects.h"
#include "game/items.h"
#include "game/random.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "math/matrix.h"

int32_t Effect_ExplodingDeath(
    int16_t item_num, int32_t mesh_bits, int16_t damage)
{
    ITEM_INFO *item = &g_Items[item_num];
    OBJECT_INFO *object = &g_Objects[item->object_number];

    FRAME_INFO *frame = Item_GetBestFrameNew(item);

    Matrix_PushUnit();
    Matrix_RotYXZ(item->rot.y, item->rot.x, item->rot.z);
    Matrix_TranslateRel(frame->offset.x, frame->offset.y, frame->offset.z);

    int32_t *packed_rotation = frame->mesh_rots;
    Matrix_RotYXZpack(*packed_rotation++);

    int32_t *bone = &g_AnimBones[object->bone_index];
#if 0
    // XXX: present in OG, removed by GLrage on the grounds that it sometimes
    // crashes.
    int16_t *extra_rotation = (int16_t*)item->data;
#endif

    int32_t bit = 1;
    if ((bit & mesh_bits) && (bit & item->mesh_bits)) {
        int16_t fx_num = Effect_Create(item->room_number);
        if (fx_num != NO_ITEM) {
            FX_INFO *fx = &g_Effects[fx_num];
            fx->room_number = item->room_number;
            fx->pos.x = (g_MatrixPtr->_03 >> W2V_SHIFT) + item->pos.x;
            fx->pos.y = (g_MatrixPtr->_13 >> W2V_SHIFT) + item->pos.y;
            fx->pos.z = (g_MatrixPtr->_23 >> W2V_SHIFT) + item->pos.z;
            fx->rot.y = (Random_GetControl() - 0x4000) * 2;
            if (item->object_number == O_TORSO) {
                fx->speed = Random_GetControl() >> 7;
                fx->fall_speed = -Random_GetControl() >> 7;
            } else {
                fx->speed = Random_GetControl() >> 8;
                fx->fall_speed = -Random_GetControl() >> 8;
            }
            fx->counter = damage;
            fx->frame_number = object->mesh_index;
            fx->object_number = O_BODY_PART;
        }
        item->mesh_bits -= bit;
    }

    for (int i = 1; i < object->nmeshes; i++) {
        int32_t bone_extra_flags = *bone++;
        if (bone_extra_flags & BEB_POP) {
            Matrix_Pop();
        }
        if (bone_extra_flags & BEB_PUSH) {
            Matrix_Push();
        }

        Matrix_TranslateRel(bone[0], bone[1], bone[2]);
        Matrix_RotYXZpack(*packed_rotation++);

#if 0
    if (extra_rotation) {
        if (bone_extra_flags & (BEB_ROT_X | BEB_ROT_Y | BEB_ROT_Z)) {
            if (bone_extra_flags & BEB_ROT_Y) {
                Matrix_RotY(*extra_rotation++);
            }
            if (bone_extra_flags & BEB_ROT_X) {
                Matrix_RotX(*extra_rotation++);
            }
            if (bone_extra_flags & BEB_ROT_Z) {
                Matrix_RotZ(*extra_rotation++);
            }
        }
    }
#endif

        bit <<= 1;
        if ((bit & mesh_bits) && (bit & item->mesh_bits)) {
            int16_t fx_num = Effect_Create(item->room_number);
            if (fx_num != NO_ITEM) {
                FX_INFO *fx = &g_Effects[fx_num];
                fx->room_number = item->room_number;
                fx->pos.x = (g_MatrixPtr->_03 >> W2V_SHIFT) + item->pos.x;
                fx->pos.y = (g_MatrixPtr->_13 >> W2V_SHIFT) + item->pos.y;
                fx->pos.z = (g_MatrixPtr->_23 >> W2V_SHIFT) + item->pos.z;
                fx->rot.y = (Random_GetControl() - 0x4000) * 2;
                if (item->object_number == O_TORSO) {
                    fx->speed = Random_GetControl() >> 7;
                    fx->fall_speed = -Random_GetControl() >> 7;
                } else {
                    fx->speed = Random_GetControl() >> 8;
                    fx->fall_speed = -Random_GetControl() >> 8;
                }
                fx->counter = damage;
                fx->object_number = O_BODY_PART;
                fx->frame_number = object->mesh_index + i;
            }
            item->mesh_bits -= bit;
        }

        bone += 3;
    }

    Matrix_Pop();

    return !(item->mesh_bits & (0x7FFFFFFF >> (31 - object->nmeshes)));
}
