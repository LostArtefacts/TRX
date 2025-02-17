#include "game/objects/traps/thors_hammer_head.h"

#include "game/items.h"
#include "game/lara/common.h"
#include "game/objects/common.h"

typedef enum {
    THOR_HAMMER_STATE_SET = 0,
    THOR_HAMMER_STATE_TEASE = 1,
    THOR_HAMMER_STATE_ACTIVE = 2,
    THOR_HAMMER_STATE_DONE = 3,
} THOR_HAMMER_STATE;

void ThorsHammerHead_Setup(OBJECT *obj)
{
    obj->collision_func = ThorsHammerHead_Collision;
    obj->draw_func = Object_DrawUnclippedItem;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

void ThorsHammerHead_Collision(
    int16_t item_num, ITEM *lara_item, COLL_INFO *coll)
{
    ITEM *const item = Item_Get(item_num);
    if (!Lara_TestBoundsCollide(item, coll->radius)) {
        return;
    }
    if (coll->enable_baddie_push
        && item->current_anim_state != THOR_HAMMER_STATE_ACTIVE) {
        Lara_Push(item, coll, false, true);
    }
}
