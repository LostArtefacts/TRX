#include "game/objects/col.h"

#include "game/lara.h"

void Object_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);

    if (!Lara_TestBoundsCollide(item, coll->radius)) {
        return;
    }

    if (!Collide_TestCollision(item, lara_item)) {
        return;
    }

    if (coll->enable_baddie_push) {
        Lara_Push(item, coll, false, true);
    }
}

void Object_Collision_Trap(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);

    if (item->status == IS_ACTIVE) {
        if (Lara_TestBoundsCollide(item, coll->radius)) {
            Collide_TestCollision(item, lara_item);
        }
    } else if (item->status != IS_INVISIBLE) {
        Object_Collision(item_num, lara_item, coll);
    }
}
