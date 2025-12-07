#include <trx/game/objects/traps/common.h>

#include <trx/game/game_buf.h>
#include <trx/game/objects/types.h>
#include <trx/game/rooms.h>

void Trap_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    GAME_VECTOR *const data =
        GameBuf_Alloc(sizeof(GAME_VECTOR), GBUF_ITEM_DATA);
    data->pos = item->pos;
    data->room_num = item->room_num;
    item->data = data;
}

void Trap_Reset(ITEM *const item)
{
    const int16_t item_num = Item_GetIndex(item);
    const GAME_VECTOR *const data = item->data;

    item->status = IS_INACTIVE;
    item->pos = data->pos;
    Item_UpdateRoom(item_num, data->room_num);

    item->goal_anim_state = TRAP_SET;
    item->current_anim_state = TRAP_SET;
    Item_SwitchToAnim(item, 0, 0);
    item->goal_anim_state = Item_GetAnim(item)->current_anim_state;
    item->current_anim_state = item->goal_anim_state;
    item->required_anim_state = TRAP_SET;
    Item_RemoveActive(item_num);
}
