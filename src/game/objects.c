#include "game/objects.h"

#include "game/collide.h"
#include "game/lara/lara.h"
#include "game/output.h"
#include "global/vars.h"

void Object_Collision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &g_Items[item_num];

    if (!Lara_TestBoundsCollide(item, coll->radius)) {
        return;
    }
    if (!Collide_TestCollision(item, lara_item)) {
        return;
    }

    if (coll->enable_baddie_push) {
        Lara_Push(item, coll, 0, 1);
    }
}

void Object_CollisionTrap(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &g_Items[item_num];

    if (item->status == IS_ACTIVE) {
        if (Lara_TestBoundsCollide(item, coll->radius)) {
            Collide_TestCollision(item, lara_item);
        }
    } else if (item->status != IS_INVISIBLE) {
        Object_Collision(item_num, lara_item, coll);
    }
}

void Object_DrawSpriteItem(ITEM_INFO *item)
{
    Output_DrawSprite(
        item->pos.x, item->pos.y, item->pos.z,
        g_Objects[item->object_number].mesh_index - item->frame_number,
        item->shade);
}
