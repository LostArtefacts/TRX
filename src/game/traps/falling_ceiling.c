#include "game/traps/falling_ceiling.h"

#include "game/collide.h"
#include "game/control.h"
#include "game/items.h"
#include "game/vars.h"

void SetupFallingCeilling(OBJECT_INFO *obj)
{
    obj->control = FallingCeilingControl;
    obj->collision = TrapCollision;
    obj->save_position = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
}

// original name: FallingCeiling
void FallingCeilingControl(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];
    if (item->current_anim_state == TRAP_SET) {
        item->goal_anim_state = TRAP_ACTIVATE;
        item->gravity_status = 1;
    } else if (item->current_anim_state == TRAP_ACTIVATE && item->touch_bits) {
        LaraItem->hit_points -= FALLING_CEILING_DAMAGE;
        LaraItem->hit_status = 1;
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
