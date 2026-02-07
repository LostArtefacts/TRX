#include <trx/game/items/utils.h>

#include <trx/game/creature.h>
#include <trx/game/effects.h>
#include <trx/game/matrix.h>
#include <trx/game/objects.h>
#include <trx/game/output.h>
#include <trx/game/random.h>
#include <trx/utils.h>
#include <trx/version.h>

void Item_TakeDamage(
    ITEM *const item, const int16_t damage, const bool hit_status)
{
    if (item->hit_points == DONT_TARGET && !Creature_IsDestructible(item)) {
        return;
    }

    item->hit_points -= damage;
    CLAMPL(item->hit_points, 0);

    if (hit_status) {
        item->hit_status = true;
    }
}

int32_t Item_Explode(
    const int16_t item_num, const int32_t mesh_bits, const int16_t damage)
{
    ITEM *const item = Item_Get(item_num);
    const OBJECT *const obj = Object_Get(item->object_id);
    if (!obj->loaded) {
        return 0;
    }

    Output_CalculateLight(item->pos, item->room_num);

    const ANIM_FRAME *const best_frame = Item_GetBestFrame(item);

    Matrix_PushUnit();
    Matrix_Rot16(item->rot);
    Matrix_TranslateRel16(best_frame->offset);
    Matrix_Rot16(best_frame->mesh_rots[0]);

    const int32_t speed_shift = item->object_id == O_TORSO ? 7 : 8;
    const bool is_tr3 = g_TRVersion == 3;

    // main mesh
    int32_t bit = 1;
    if ((mesh_bits & bit) && (item->mesh_bits & bit)) {
        const int16_t effect_num = Effect_Create(item->room_num);
        if (effect_num != NO_EFFECT) {
            EFFECT *const effect = Effect_Get(effect_num);
            effect->pos.x = item->pos.x + (g_MatrixPtr->_03 >> W2V_SHIFT);
            effect->pos.y = item->pos.y + (g_MatrixPtr->_13 >> W2V_SHIFT);
            effect->pos.z = item->pos.z + (g_MatrixPtr->_23 >> W2V_SHIFT);
            effect->rot.y = (Random_GetControl() - 0x4000) * 2;
            effect->room_num = item->room_num;
            effect->speed = Random_GetControl() >> speed_shift;
            effect->fall_speed = -Random_GetControl() >> speed_shift;
            effect->counter =
                is_tr3 ? ((damage << 2) | (Random_GetControl() & 3)) : damage;
            effect->object_id = O_BODY_PART;
            effect->frame_num = obj->mesh_idx;
            effect->shade = Output_GetLightAdder() - 0x300;
        }
        item->mesh_bits &= ~bit;
    }

    // additional meshes
    const int16_t *extra_rotation = (int16_t *)item->data;
    for (int32_t i = 1; i < obj->mesh_count; i++) {
        const ANIM_BONE *const bone = Object_GetBone(obj, i - 1);
        if (bone->matrix_pop) {
            Matrix_Pop();
        }
        if (bone->matrix_push) {
            Matrix_Push();
        }

        Matrix_TranslateRel32(bone->pos);
        Matrix_Rot16(best_frame->mesh_rots[i]);
        Object_ApplyExtraRotation(&extra_rotation, bone->rot, false);

        bit <<= 1;
        if ((mesh_bits & bit) && (item->mesh_bits & bit)) {
            const int16_t effect_num = Effect_Create(item->room_num);
            if (effect_num != NO_EFFECT) {
                EFFECT *const effect = Effect_Get(effect_num);
                effect->pos.x = item->pos.x + (g_MatrixPtr->_03 >> W2V_SHIFT);
                effect->pos.y = item->pos.y + (g_MatrixPtr->_13 >> W2V_SHIFT);
                effect->pos.z = item->pos.z + (g_MatrixPtr->_23 >> W2V_SHIFT);
                effect->rot.y = (Random_GetControl() - 0x4000) * 2;
                effect->room_num = item->room_num;
                effect->speed = Random_GetControl() >> speed_shift;
                effect->fall_speed = -Random_GetControl() >> speed_shift;
                effect->counter = is_tr3
                    ? ((damage << 2) | (Random_GetControl() & 3))
                    : damage;
                effect->object_id = O_BODY_PART;
                effect->frame_num = obj->mesh_idx + i;
                effect->shade = Output_GetLightAdder() - 0x300;
            }
            item->mesh_bits &= ~bit;
        }
    }

    Matrix_Pop();

    return !(item->mesh_bits & (0x7FFFFFFF >> (31 - obj->mesh_count)));
}

bool Item_ShouldSpawnBlood(const ITEM *const item)
{
    if (item == nullptr) {
        return true;
    }

    const OBJECT *const obj = Object_Get(item->object_id);
    if (obj->should_spawn_blood_func != nullptr) {
        return obj->should_spawn_blood_func(item);
    }

    return true;
}
