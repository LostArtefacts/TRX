#include "game/lara/control.h"

#include "game/lara.h"

void Lara_DismountVehicle(void)
{
#if TR_VERSION >= 2
    ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara_info = Lara_GetLaraInfo();

    if (lara_info->vehicle_item_num != NO_ITEM) {
        ITEM *const vehicle = Item_Get(lara_info->vehicle_item_num);
        Item_SwitchToAnim(vehicle, 0, 0);
        lara_info->vehicle_item_num = NO_ITEM;

        lara_item->current_anim_state = LS_STOP;
        lara_item->goal_anim_state = LS_STOP;
        Item_SwitchToAnim(lara_item, LA_STAND_STILL, 0);

        lara_item->rot.x = 0;
        lara_item->rot.z = 0;
    }
#endif
}
