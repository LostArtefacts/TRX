#include "game/objects/general/cabin.h"

#include "game/items.h"
#include "game/objects/common.h"
#include "game/room.h"
#include "global/vars.h"

void Cabin_Setup(OBJECT_INFO *obj)
{
    obj->control = Cabin_Control;
    obj->draw_routine = Object_DrawUnclippedItem;
    obj->collision = Object_Collision;
    obj->save_anim = 1;
    obj->save_flags = 1;
}

void Cabin_Control(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];

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
        g_FlipMapTable[3] = IF_CODE_BITS;
        Room_FlipMap();
        Item_Kill(item_num);
    }

    Item_Animate(item);
}
