#include "game/vars.h"
#include "game/pickup.h"

int32_t KeyTrigger(int16_t item_num)
{
    ITEM_INFO* item = &Items[item_num];
    if (item->status == IS_ACTIVE && Lara.gun_status == LGS_ARMLESS) {
        item->status = IS_DEACTIVATED;
        return 1;
    }
    return 0;
}

void T1MInjectGamePickup()
{
    INJECT(0x00433EA0, KeyTrigger);
}
