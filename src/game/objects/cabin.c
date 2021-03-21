#include "game/objects/cabin.h"

#include "game/collide.h"
#include "game/control.h"
#include "game/draw.h"
#include "game/items.h"
#include "global/vars.h"

void SetupCabin(OBJECT_INFO *obj)
{
    obj->control = CabinControl;
    obj->draw_routine = DrawUnclippedItem;
    obj->collision = ObjectCollision;
    obj->save_anim = 1;
    obj->save_flags = 1;
}

void CabinControl(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];

    if ((item->flags & IF_CODE_BITS) == IF_CODE_BITS) {
        switch (item->current_anim_state) {
        case CABIN_START:
            item->goal_anim_state = CABIN_DROP1;
            break;
        case CABIN_DROP1:
            item->goal_anim_state = CABIN_DROP2;
            break;
        case CABIN_DROP2:
            item->goal_anim_state = CABIN_DROP3;
            break;
        }
        item->flags = 0;
    }

    if (item->current_anim_state == CABIN_FINISH) {
        FlipMapTable[3] = IF_CODE_BITS;
        FlipMap();
        KillItem(item_num);
    }

    AnimateItem(item);
}
