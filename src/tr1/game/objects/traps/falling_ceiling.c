#include "game/items.h"
#include "game/lara/common.h"
#include "game/objects/common.h"

#define FALLING_CEILING_DAMAGE 300

static void M_Setup(OBJECT *obj);
static void M_Control(int16_t item_num);

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->collision_func = Object_CollisionTrap;
    obj->save_position = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (item->current_anim_state == TRAP_SET) {
        item->goal_anim_state = TRAP_ACTIVATE;
        item->gravity = 1;
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
        item->gravity = 0;
    }
}

REGISTER_OBJECT(O_FALLING_CEILING_1, M_Setup)
REGISTER_OBJECT(O_FALLING_CEILING_2, M_Setup)
