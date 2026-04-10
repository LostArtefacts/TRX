#include <trx/game/objects.h>
#include <trx/game/rooms.h>

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    if (!Item_IsTriggerActive(item)) {
        Item_Kill(item_num);
        return;
    }

    Item_Animate(item);

    int16_t room_num = item->room_num;
    Room_GetSector(item->pos, &room_num);
    Item_UpdateRoom(item_num, room_num);
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->control_func = M_Control;
    obj->save_position = true;
    obj->save_flags = true;
}

REGISTER_OBJECT(O_DISPOSABLE_ANIMATING_1, M_Setup)
REGISTER_OBJECT(O_DISPOSABLE_ANIMATING_2, M_Setup)
REGISTER_OBJECT(O_DISPOSABLE_ANIMATING_3, M_Setup)
REGISTER_OBJECT(O_DISPOSABLE_ANIMATING_4, M_Setup)
REGISTER_OBJECT(O_DISPOSABLE_ANIMATING_5, M_Setup)
REGISTER_OBJECT(O_DISPOSABLE_ANIMATING_6, M_Setup)
REGISTER_OBJECT(O_DISPOSABLE_ANIMATING_7, M_Setup)
REGISTER_OBJECT(O_DISPOSABLE_ANIMATING_8, M_Setup)
REGISTER_OBJECT(O_DISPOSABLE_ANIMATING_9, M_Setup)
REGISTER_OBJECT(O_DISPOSABLE_ANIMATING_10, M_Setup)
