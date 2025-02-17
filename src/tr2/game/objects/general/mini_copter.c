#include "game/objects/general/mini_copter.h"

#include "game/items.h"
#include "game/objects/common.h"
#include "game/room.h"
#include "game/sound.h"

#include <libtrx/game/lara/common.h>
#include <libtrx/utils.h>

void MiniCopter_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    const ITEM *const lara_item = Lara_GetItem();

    item->pos.z += 100;

    XYZ_32 pos = lara_item->pos;
    pos.x += ((item->pos.x - lara_item->pos.x) >> 2);
    pos.y += ((item->pos.y - lara_item->pos.y) >> 2);
    pos.z += ((item->pos.z - lara_item->pos.z) >> 2);
    Sound_Effect(SFX_HELICOPTER_LOOP, &pos, SPM_NORMAL);

    if (ABS(item->pos.z - lara_item->pos.z) > WALL_L * 30) {
        Item_Kill(item_num);
    }

    Item_Animate(item);

    int16_t room_num = item->room_num;
    Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
    if (room_num != item->room_num) {
        Item_NewRoom(item_num, room_num);
    }
}

void MiniCopter_Setup(void)
{
    OBJECT *const obj = Object_Get(O_MINI_COPTER);
    obj->control_func = MiniCopter_Control;
    obj->save_position = 1;
    obj->save_flags = 1;
}
