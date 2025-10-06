#include "game/lara.h"
#include "game/objects/common.h"
#include "game/spawn.h"

#include <libtrx/game/random.h>

#define PENDULUM_DAMAGE 100

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    if (Item_IsTriggerActive(item)) {
        if (item->current_anim_state == TRAP_SET) {
            item->goal_anim_state = TRAP_WORKING;
        }
    } else {
        if (item->current_anim_state == TRAP_WORKING) {
            item->goal_anim_state = TRAP_SET;
        }
    }

    if (item->current_anim_state == TRAP_WORKING && item->touch_bits) {
        Lara_TakeDamage(PENDULUM_DAMAGE, true);
        const ITEM *const lara_item = Lara_GetItem();
        const int32_t x =
            lara_item->pos.x + (Random_GetControl() - 0x4000) / 256;
        const int32_t z =
            lara_item->pos.z + (Random_GetControl() - 0x4000) / 256;
        const int32_t y = lara_item->pos.y - Random_GetControl() / 44;
        const int32_t d = lara_item->rot.y + (Random_GetControl() - 0x4000) / 8;
        Spawn_Blood(x, y, z, lara_item->speed, d, lara_item->room_num);
    }

    const SECTOR *const sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &item->room_num);
    item->floor = Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);

    Item_Animate(item);
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->collision_func = Object_Collision_Trap;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->save_flags = true;
    obj->save_anim = true;
}

REGISTER_OBJECT(O_PENDULUM_1, M_Setup)
