#include "game/objects.h"

#include "game/lara/lara.h"
#include "game/sphere.h"
#include "global/vars.h"

void Object_Collision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &g_Items[item_num];

    if (!Lara_TestBoundsCollide(item, coll->radius)) {
        return;
    }
    if (!TestCollision(item, lara_item)) {
        return;
    }

    if (coll->enable_baddie_push) {
        Lara_Push(item, coll, 0, 1);
    }
}
