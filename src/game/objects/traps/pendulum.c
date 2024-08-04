#include "game/objects/traps/pendulum.h"

#include "game/effects/blood.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/objects/common.h"
#include "game/random.h"
#include "game/room.h"
#include "global/const.h"
#include "global/vars.h"

#include <stdbool.h>

#define PENDULUM_DAMAGE 100

void Pendulum_Setup(OBJECT_INFO *obj)
{
    obj->control = Pendulum_Control;
    obj->collision = Object_CollisionTrap;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

void Pendulum_Control(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];

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
        int32_t x = g_LaraItem->pos.x + (Random_GetControl() - 0x4000) / 256;
        int32_t z = g_LaraItem->pos.z + (Random_GetControl() - 0x4000) / 256;
        int32_t y = g_LaraItem->pos.y - Random_GetControl() / 44;
        int32_t d = g_LaraItem->rot.y + (Random_GetControl() - 0x4000) / 8;
        Effect_Blood(x, y, z, g_LaraItem->speed, d, g_LaraItem->room_number);
    }

    const SECTOR_INFO *const sector = Room_GetSector(
        item->pos.x, item->pos.y, item->pos.z, &item->room_number);
    item->floor = Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);

    Item_Animate(item);
}
