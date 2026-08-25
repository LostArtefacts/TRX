#include <trx/game/objects.h>

typedef enum {
    M_STATE_EMPTY,
    M_STATE_SET,
    M_STATE_MOVE,
    M_STATE_STOP,
} M_STATE;

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    switch (item->current_anim_state) {
    case M_STATE_SET:
        item->goal_anim_state = M_STATE_MOVE;
        break;
    case M_STATE_MOVE:
        item->goal_anim_state = M_STATE_STOP;
        break;
    case M_STATE_STOP:
        Item_Destroy(item_num);
        break;
    }

    Item_Animate(item);
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->save_flags = true;
    obj->save_anim = true;
    obj->save_position = true;
}

REGISTER_OBJECT(O_MOORED_BOAT, M_Setup)
