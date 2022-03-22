#include "game/traps/falling_ceiling.h"

#include "game/collide.h"
#include "game/control.h"
#include "game/items.h"
#include "global/vars.h"

#define FALLING_CEILING_DAMAGE 300

void FallingCeiling_Setup(OBJECT_INFO *obj)
{
    obj->control = FallingCeiling_Control;
    obj->collision = TrapCollision;
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
        g_LaraItem->hit_points -= FALLING_CEILING_DAMAGE;
        g_LaraItem->hit_status = 1;
    }
    AnimateItem(item);
    if (item->status == IS_DEACTIVATED) {
        RemoveActiveItem(item_num);
    } else if (
        item->current_anim_state == TRAP_ACTIVATE
        && item->pos.y >= item->floor) {
        item->goal_anim_state = TRAP_WORKING;
        item->pos.y = item->floor;
        item->fall_speed = 0;
        item->gravity_status = 0;
    }
}
