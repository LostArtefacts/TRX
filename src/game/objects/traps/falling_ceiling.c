#include "game/objects/traps/falling_ceiling.h"

#include "game/items.h"
#include "game/lara.h"
#include "game/objects/common.h"

#include <stdbool.h>

#define FALLING_CEILING_DAMAGE 300

void FallingCeiling_Setup(OBJECT_INFO *obj)
{
    obj->control = FallingCeiling_Control;
    obj->collision = Object_CollisionTrap;
    obj->save_position = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
}

void FallingCeiling_Control(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    if (item->current_anim_state == TRAP_SET) {
        item->goal_anim_state = TRAP_ACTIVATE;
        item->gravity_status = 1;
    } else if (item->current_anim_state == TRAP_ACTIVATE && item->touch_bits) {
        Lara_TakeDamage(FALLING_CEILING_DAMAGE, true);
    }
    Item_Animate(item);
    if (item->status == IS_DEACTIVATED) {
        Item_RemoveActive(item_num);
    } else if (
        item->current_anim_state == TRAP_ACTIVATE
        && item->pos.y >= item->floor) {
        item->goal_anim_state = TRAP_WORKING;
        item->pos.y = item->floor;
        item->fall_speed = 0;
        item->gravity_status = 0;
    }
}
