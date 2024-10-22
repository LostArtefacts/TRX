#include "game/objects/general/lift.h"

#include "global/funcs.h"

void __cdecl Lift_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    LIFT_INFO *const lift_data =
        game_malloc(sizeof(LIFT_INFO), GBUF_TEMP_ALLOC);
    lift_data->start_height = item->pos.y;
    lift_data->wait_time = 0;

    item->data = lift_data;
}
