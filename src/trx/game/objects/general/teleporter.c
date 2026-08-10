#include <trx/game/camera.h>
#include <trx/game/interpolation.h>
#include <trx/game/items.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/rooms.h>

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (!Item_IsTriggerActive(item)) {
        return;
    }

    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(item->pos, &room_num);
    const int32_t height = Room_GetHeight(sector, item->pos);

    ITEM *const lara_item = Lara_GetItem();
    lara_item->pos.x = item->pos.x;
    lara_item->pos.y = height;
    lara_item->pos.z = item->pos.z;
    lara_item->rot.y = item->rot.y + DEG_180;
    lara_item->floor = height;
    Item_UpdateRoom(Item_GetIndex(lara_item), room_num);

    g_Camera.fixed_camera = true;
    Interpolation_CommitLara();

    Item_RemoveSimulated(item_num);
    item->trigger.mask = 0;
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->draw_func = nullptr;
    obj->save_flags = true;
}

REGISTER_OBJECT(O_TELEPORTER, M_Setup)
