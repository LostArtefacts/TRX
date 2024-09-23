#include "game/objects/traps/thors_hammer_head.h"

#include "game/items.h"
#include "game/lara/common.h"
#include "game/objects/common.h"

typedef enum {
    THS_SET = 0,
    THS_TEASE = 1,
    THS_ACTIVE = 2,
    THS_DONE = 3,
} THOR_HAMMER_STATE;

void ThorsHammerHead_Setup(OBJECT *obj)
{
    obj->collision = ThorsHammerHead_Collision;
    obj->draw_routine = Object_DrawUnclippedItem;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

void ThorsHammerHead_Collision(
    int16_t item_num, ITEM *lara_item, COLL_INFO *coll)
{
    ITEM *item = &g_Items[item_num];
    if (!Lara_TestBoundsCollide(item, coll->radius)) {
        return;
    }
    if (coll->enable_baddie_push && item->current_anim_state != THS_ACTIVE) {
        Lara_Push(item, coll, false, true);
    }
}
