#include <trx/game/objects.h>
#include <trx/game/rooms.h>

static void M_UpdateTrigger(const ITEM *const item, bool enabled)
{
    int16_t room_num = item->room_num;
    SECTOR *const sector = Room_GetSector(item->pos, &room_num);
    if (sector->trigger != nullptr) {
        sector->trigger->enabled = enabled;
    }
}

static void M_Initialise(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);
    const bool enabled = Item_IsTriggerActiveRO(item);
    M_UpdateTrigger(item, enabled);
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    const bool enabled = Item_IsTriggerActive(item);
    M_UpdateTrigger(item, enabled);
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->draw_func = nullptr;
    obj->save_flags = true;
}

REGISTER_OBJECT(O_TRIGGER_GATE, M_Setup)
