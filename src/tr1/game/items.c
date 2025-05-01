#include "game/items.h"

#include "game/carrier.h"
#include "game/room.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/game.h>
#include <libtrx/game/interpolation.h>

void Item_Control(void)
{
    int16_t item_num = Item_GetNextActive();
    while (item_num != NO_ITEM) {
        ITEM *item = Item_Get(item_num);
        const OBJECT *const obj = Object_Get(item->object_id);
        if (obj->control_func != nullptr) {
            obj->control_func(item_num);
        }
        item_num = item->next_active;
    }

    Carrier_AnimateDrops();
}
