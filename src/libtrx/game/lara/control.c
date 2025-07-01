#include "game/lara/control.h"

#include "game/lara.h"
#include "game/rooms.h"

#define M_MAX_BADDIE_COLLISION 20

// TODO: make private
void Lara_BaddieCollision(ITEM *const lara_item, COLL_INFO *const coll)
{
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    lara_info->hit_direction = -1;
    lara_item->hit_status = false;
    if (lara_item->hit_points <= 0) {
        return;
    }

    int16_t roomies[M_MAX_BADDIE_COLLISION];
    const int32_t roomies_count = Room_GetAdjoiningRooms(
        lara_item->room_num, roomies, M_MAX_BADDIE_COLLISION);

    for (int32_t i = 0; i < roomies_count; i++) {
        int16_t item_num = Room_Get(roomies[i])->item_num;
        while (item_num != NO_ITEM) {
            const ITEM *const item = Item_Get(item_num);

            // the collision routine can destroy the item - need to store the
            // next item beforehand
            const int16_t next_item_num = item->next_item;

            if (item->collidable && item->status != IS_INVISIBLE) {
                const OBJECT *const obj = Object_Get(item->object_id);
                if (obj->collision_func != nullptr) {
                    // clang-format off
                    const XYZ_32 d = {
                        .x = lara_item->pos.x - item->pos.x,
                        .y = lara_item->pos.y - item->pos.y,
                        .z = lara_item->pos.z - item->pos.z,
                    };
                    if (d.x > -CREATURE_TARGET_DIST && d.x < CREATURE_TARGET_DIST &&
                        d.y > -CREATURE_TARGET_DIST && d.y < CREATURE_TARGET_DIST &&
                        d.z > -CREATURE_TARGET_DIST && d.z < CREATURE_TARGET_DIST) {
                        obj->collision_func(item_num, lara_item, coll);
                    }
                    // clang-format on
                }
            }

            item_num = next_item_num;
        }
    }

    if (lara_info->hit_effect_count != 0 && lara_info->hit_effect != nullptr
        && coll->enable_hit) {
        const int32_t dx = lara_info->hit_effect->pos.x - lara_item->pos.x;
        const int32_t dz = lara_info->hit_effect->pos.z - lara_item->pos.z;
        Lara_TakeHit(lara_item, dx, dz);
        lara_info->hit_effect_count--;
    }

    if (lara_info->hit_direction == -1) {
        lara_info->hit_frame = 0;
    }
}

void Lara_DismountVehicle(void)
{
#if TR_VERSION >= 2
    ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara_info = Lara_GetLaraInfo();

    if (lara_info->vehicle_item_num != NO_ITEM) {
        ITEM *const vehicle = Item_Get(lara_info->vehicle_item_num);
        Item_SwitchToAnim(vehicle, 0, 0);
        lara_info->vehicle_item_num = NO_ITEM;

        lara_item->current_anim_state = LS_STOP;
        lara_item->goal_anim_state = LS_STOP;
        Item_SwitchToAnim(lara_item, LA_STAND_STILL, 0);

        lara_item->rot.x = 0;
        lara_item->rot.z = 0;
    }
#endif
}
