#include "game/items.h"

#include <libtrx/game/carrier.h>

void Item_Control(void)
{
    int16_t item_num = Item_GetNextActive();
    while (item_num != NO_ITEM) {
        const ITEM *const item = Item_Get(item_num);
        const int16_t next = item->next_active;
        const OBJECT *obj = Object_Get(item->object_id);
        if (!(item->flags & IF_KILLED) && obj->control_func != nullptr) {
            obj->control_func(item_num);
        }
        item_num = next;
    }

    Carrier_AnimateDrops();
}
