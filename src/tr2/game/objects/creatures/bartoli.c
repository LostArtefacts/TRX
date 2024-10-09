#include "game/objects/creatures/bartoli.h"

#include "game/items.h"
#include "game/shell.h"
#include "global/vars.h"

void __cdecl Bartoli_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    item->pos.x -= 2 * STEP_L;
    item->pos.z -= 2 * STEP_L;

    const int16_t item_dragon_back_num = Item_Create();
    const int16_t item_dragon_front_num = Item_Create();
    if (item_dragon_back_num == NO_ITEM || item_dragon_front_num == NO_ITEM) {
        Shell_ExitSystem("Unable to create dragon");
    }

    ITEM *const item_dragon_back = Item_Get(item_dragon_back_num);
    item_dragon_back->object_id = O_DRAGON_BACK;
    item_dragon_back->pos.x = item->pos.x;
    item_dragon_back->pos.y = item->pos.y;
    item_dragon_back->pos.z = item->pos.z;
    item_dragon_back->rot.y = item->rot.y;
    item_dragon_back->room_num = item->room_num;
    item_dragon_back->flags = IF_INVISIBLE;
    item_dragon_back->shade_1 = -1;
    Item_Initialise(item_dragon_back_num);
    item_dragon_back->mesh_bits = 0x1FFFFF;

    ITEM *const item_dragon_front = Item_Get(item_dragon_front_num);
    item_dragon_front->object_id = O_DRAGON_FRONT;
    item_dragon_front->pos.x = item->pos.x;
    item_dragon_front->pos.y = item->pos.y;
    item_dragon_front->pos.z = item->pos.z;
    item_dragon_front->rot.y = item->rot.y;
    item_dragon_front->room_num = item->room_num;
    item_dragon_front->flags = IF_INVISIBLE;
    item_dragon_front->shade_1 = -1;
    Item_Initialise(item_dragon_front_num);
    item_dragon_back->data = (void *)(intptr_t)item_dragon_front_num;

    item->data = (void *)(intptr_t)item_dragon_back_num;

    g_LevelItemCount += 2;
}
